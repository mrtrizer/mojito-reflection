#include <cstdio>
#include <iostream>
#include <fstream>
#include <thread>
#include <stdexcept>
#include <unistd.h>
#include <sstream>
#include <unordered_set>

#include <clang/Frontend/FrontendActions.h>
#include <llvm/ADT/StringRef.h>

#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "generate_method.h"
#include "CustomCompilationDB.hpp"
#include "ClassParser.hpp"
#include "CompillerArgs.hpp"
#include "GeneratorArgs.hpp"

using namespace boost;
using namespace std;
using namespace llvm;
using namespace clang::tooling;
using namespace clang;
using namespace clang::ast_matchers;

int runClang(const std::string& command)
{
    std::cout << command << std::endl;
    
    process::ipstream pipe_stream;
    process::child c(command, process::std_out > pipe_stream);

    std::string line;

    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
        std::cout << line << std::endl;

    c.wait();
    return c.exit_code();
}

struct TypeReflection {
    std::string typeName;
    GeneratedMethods methods;
};

struct ReflectionUnit {
    filesystem::path originalPath;
    std::string name;
    std::vector<TypeReflection> reflectedTypes;
};

std::string generateWrapperCpp(ReflectionUnit reflectionUnit) {
    std::stringstream ss;
    
    ss << "#include <" << reflectionUnit.originalPath.string() << ">" << std::endl;
    ss << "#include <Type.hpp>" << std::endl;
    ss << "#include <BasicTypesReflection.hpp>" << std::endl;
    ss << std::endl;
    ss << "using namespace flappy;" << std::endl;
    ss << std::endl;
    ss << "void " << reflectionUnit.name << "(Reflection& reflection) {" << std::endl;
    for (const auto& type : reflectionUnit.reflectedTypes) {
        ss << "  reflection.registerType<" << type.typeName << ">(\"" << type.typeName  << "\")" << std::endl;
        ss << type.methods.methodBodies << ";" << std::endl;
    }
    ss << "}" << std::endl;
    ss << std::endl;
    
    return ss.str();
}

void writeTextFile(const filesystem::path& filePath, const std::string& data) {
    std::ofstream textFile;
    textFile.open(filePath.string());
    textFile << data;
    textFile.close();
}

std::tuple<std::string, std::string> splitDefine(const std::string& sourceStr) {
    std::string::size_type eq = sourceStr.find('=');
    std::string value = eq != std::string::npos ? sourceStr.substr(eq + 1) : "";
    return {sourceStr.substr(2, eq - 2), value};
}

CompillerArgs parseCompillerArgs(const boost::filesystem::path& compillerPath, const std::vector<std::string>& arguments) {
    CompillerArgs compillerArgs;
    
    enum {
        INCLUDE_PATH,
        UNKNOWN,
        DEFINE,
        OUTPUT,
        XLINKER_OPTION
    } state = UNKNOWN;
    
    for (const auto& arg : arguments) {
        if (state == UNKNOWN && arg.find("-I") == 0) {
            state = arg.size() > 2 ? compillerArgs.addIncludePath(arg.substr(2)), UNKNOWN : INCLUDE_PATH;
        } else if (state == UNKNOWN && arg.find("-D") == 0) {
            if (arg.size() > 2) {
                auto split = splitDefine(arg);
                compillerArgs.addDefine(std::get<0>(split), std::get<1>(split));
            } else {
                state = DEFINE;
            }
        } else if (state == UNKNOWN && arg.size() == 2 && arg.find("-o") == 0) {
            state = OUTPUT;
        } else if (state == UNKNOWN && arg.find("-Xlinker") == 0) {
            state = XLINKER_OPTION;
        } else if (state == UNKNOWN && arg.find(".cpp") != std::string::npos) {
            compillerArgs.addCppInputFile(arg);
        } else if (state == UNKNOWN && arg.find(".o") != std::string::npos) {
            compillerArgs.addObjInputFile(arg);
        } else if (state == INCLUDE_PATH) {
            compillerArgs.addIncludePath(arg);
            state = UNKNOWN;
        } else if (state == DEFINE) {
            auto split = splitDefine(arg);
            compillerArgs.addDefine(std::get<0>(split), std::get<1>(split));
            state = UNKNOWN;
        } else if (state == OUTPUT) {
            compillerArgs.setOutput(arg);
            state = UNKNOWN;
        } else if (state == XLINKER_OPTION) {
            compillerArgs.addLinkerOption(arg);
            state = UNKNOWN;
        } else {
            compillerArgs.addUnrecognizedArg(arg);
        }
    }
    
    return compillerArgs;
}

class ReflectionDB {
public:
    ReflectionDB(const boost::filesystem::path& reflectionDbFilePath)
        : m_reflectionDbFilePath(reflectionDbFilePath)
    {
        using namespace boost::property_tree;
    
        std::ifstream ifs (reflectionDbFilePath.string());
        
        if (!ifs.eof() && !ifs.fail()) {
            ptree pt;

            read_json(ifs, pt);
            
            for (const auto& value : pt.get_child(reflectedFilesKey)) {
                auto reflectedFileData = value.second;
                ReflectedFile reflectedFile;
                reflectedFile.cppFilePath = reflectedFileData.get_child(cppFilePathKey).get_value<std::string>();
                reflectedFile.outFilePath = reflectedFileData.get_child(outFilePathKey).get_value<std::string>();
                reflectedFile.functionName = reflectedFileData.get_child(functionNameKey).get_value<std::string>();
                m_reflectedFiles.emplace_back(reflectedFile);
            }
        }
    }
    
    struct ReflectedFile {
        boost::filesystem::path cppFilePath;
        boost::filesystem::path outFilePath;
        std::string functionName;
    };
    
    const std::vector<ReflectedFile>& reflectedFiles() const { return m_reflectedFiles; }
    
    using FilePath = boost::filesystem::path;
    
    void addReflectedFile(const FilePath& fileName, const FilePath& reflectedFileName, const std::string& funcName) {
        m_reflectedFiles.emplace_back(ReflectedFile{fileName, reflectedFileName, funcName});
    }
    
    void save() {
        using namespace boost::property_tree;
        
        ptree pt;
        
        for (const auto& reflectedFile : m_reflectedFiles) {
            ptree reflectedFileData;
            reflectedFileData.add(cppFilePathKey, reflectedFile.cppFilePath.string());
            reflectedFileData.add(outFilePathKey, reflectedFile.outFilePath.string());
            reflectedFileData.add(functionNameKey, reflectedFile.functionName);
            pt.add_child(reflectedFilesKey, reflectedFileData);
        }
        
        write_json(m_reflectionDbFilePath.string(), pt);
    }
    
private:
    boost::filesystem::path m_reflectionDbFilePath;
    std::vector<ReflectedFile> m_reflectedFiles;
    inline static const char* reflectedFilesKey = "reflected_files";
    inline static const char* cppFilePathKey = "cpp_file_path";
    inline static const char* outFilePathKey = "out_file_path";
    inline static const char* functionNameKey = "function_name";
};

std::string generateReflectionCpp(const ReflectionDB& reflectionDB) {
    std::stringstream ss;
    
    ss << "#include<Reflection.hpp>" << std::endl;
    ss << std::endl;
    ss << "using namespace flappy;" << std::endl;
    ss << std::endl;
    for (auto file : reflectionDB.reflectedFiles()) {
        if (filesystem::exists(file.cppFilePath))
            ss << "void " << file.functionName << "(Reflection&);" << std::endl;
    }
    ss << "Reflection generateReflection() {" << std::endl;
    ss << "    Reflection reflectionDB;" << std::endl;
    
    for (auto file : reflectionDB.reflectedFiles()) {
        if (filesystem::exists(file.cppFilePath))
            ss << "    " << file.functionName << "(reflectionDB);" << std::endl;
    }
    
    ss << "    return reflectionDB;" << std::endl;
    ss << "}" << std::endl;
    
    return ss.str();
}

int main(int argc, const char *argv[])
{
    GeneratorArgs generatorArgs(std::vector<std::string>{argv + 1, argv + argc});
    
    auto compillerArgs = parseCompillerArgs(generatorArgs.compillerPath(), generatorArgs.unrecognized());

    auto reflectionDBPath = generatorArgs.reflectionOutPath();
    reflectionDBPath.append("ReflectionDB.json");
    auto reflectionDB = ReflectionDB(reflectionDBPath);

    if (!compillerArgs.cppInputFiles().empty()) {
        std::cout << "Building sources" << std::endl;
    
        CustomCompilationDatabase compilationDatabase(generatorArgs.compillerPath(), compillerArgs);

        std::vector<std::string> cppFiles;
        std::transform(compillerArgs.cppInputFiles().begin(), compillerArgs.cppInputFiles().end(), std::back_inserter(cppFiles),
            [](const auto& input) { return input.string(); });
        ClangTool tool(compilationDatabase, ArrayRef<std::string>(cppFiles));

        std::unordered_map<std::string, ReflectionUnit> generatedCppFiles;

        MatchFinder finder;
        auto methodMatcher = cxxRecordDecl(isClass(), isDefinition()).bind("classes");
        ClassParser classParser([&generatedCppFiles](const CXXRecordDecl* classDecl, const std::string& fileName, const std::string& className) {
            auto originalFilePath = filesystem::absolute(fileName).normalize();
            filesystem::path newFilePath;
            newFilePath.append(originalFilePath.parent_path().string());
            newFilePath.append(originalFilePath.stem().string() + "_r" + originalFilePath.extension().string());
            newFilePath.normalize();
            auto typeReflection = TypeReflection{ className, processMethods(classDecl, className) };
            auto iter = generatedCppFiles.find(newFilePath.string());
            if (iter == generatedCppFiles.end()) {
                auto reflectionUnitName = "register_" + originalFilePath.stem().string();
                auto reflectionUnit = ReflectionUnit { originalFilePath, reflectionUnitName, {typeReflection} };
                generatedCppFiles.emplace(newFilePath.string(), reflectionUnit);
            } else {
                iter->second.reflectedTypes.emplace_back(typeReflection);
            }
        });
        finder.addMatcher(methodMatcher, &classParser);

        tool.run(newFrontendActionFactory(&finder).get());
        
        std::vector<filesystem::path> generatedCppFilePaths;

        for (auto cppFile : generatedCppFiles) {
            writeTextFile(cppFile.first, generateWrapperCpp(cppFile.second));
            generatedCppFilePaths.emplace_back(cppFile.first);
            reflectionDB.addReflectedFile(cppFile.second.originalPath, cppFile.first, cppFile.second.name);
        }
        
        compillerArgs.setCppInputFiles(generatedCppFilePaths);

        {
            auto reflectionIncludesPath = generatorArgs.reflectionIncludesPath();
            compillerArgs.addIncludePath(reflectionIncludesPath);
            reflectionIncludesPath.append("/../../Utility/src");
            compillerArgs.addIncludePath(reflectionIncludesPath);
        }
    }
    
    if (compillerArgs.output().extension() != ".o") {
        std::cout << "Linking" << std::endl;
        auto reflectionCppData = generateReflectionCpp(reflectionDB);
        auto reflectionCppPath = generatorArgs.reflectionOutPath();
        reflectionCppPath.append("Reflection.cpp");
        compillerArgs.addCppInputFile(reflectionCppPath);
        filesystem::create_directories(generatorArgs.reflectionOutPath());
        writeTextFile(reflectionCppPath, reflectionCppData);
    }

    return runClang(compillerArgs.serialize());
}
