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
#include <clang/Tooling/Tooling.h>

#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "CustomCompilationDB.hpp"
#include "ClassParser.hpp"
#include "CompillerArgs.hpp"
#include "GeneratorArgs.hpp"
#include "PersistentReflectionDB.hpp"

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
    ss << "void " << reflectionUnit.name << "(flappy::Reflection& reflection) {" << std::endl;
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

std::tuple<std::string, std::string> splitArgument(const std::string& sourceStr) {
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
        XLINKER_OPTION,
        STANDARD
    } state = UNKNOWN;
    
    for (const auto& arg : arguments) {
        if (state == UNKNOWN && arg.find("-I") == 0) {
            state = arg.size() > 2 ? compillerArgs.addIncludePath(arg.substr(2)), UNKNOWN : INCLUDE_PATH;
        } else if (state == UNKNOWN && arg.find("-D") == 0) {
            if (arg.size() > 2) {
                auto split = splitArgument(arg);
                compillerArgs.addDefine(std::get<0>(split), std::get<1>(split));
            } else {
                state = DEFINE;
            }
        } else if (state == UNKNOWN && arg.find("-std") != std::string::npos) {
            auto split = splitArgument(arg);
            if (!std::get<1>(split).empty()) {
                compillerArgs.setCppStandard(std::get<1>(split));
            } else {
                state = STANDARD;
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
            auto split = splitArgument(arg);
            compillerArgs.addDefine(std::get<0>(split), std::get<1>(split));
            state = UNKNOWN;
        } else if (state == OUTPUT) {
            compillerArgs.setOutput(arg);
            state = UNKNOWN;
        } else if (state == XLINKER_OPTION) {
            compillerArgs.addLinkerOption(arg);
            state = UNKNOWN;
        } else if (state == STANDARD) {
            compillerArgs.setCppStandard(arg);
            state = UNKNOWN;
        } else {
            compillerArgs.addUnrecognizedArg(arg);
        }
    }
    
    return compillerArgs;
}

std::string serializeCompillerArgs(const filesystem::path& compillerPath, const CompillerArgs& compillerArgs) {
    std::stringstream ss;
    
    auto arguments = compillerArgs.clangArguments();
    
    ss << compillerPath.string() << ' ';
    
    for (const auto& arg : compillerArgs.unrecognizedArgs())
        ss << arg << ' ';
    
    for (const auto& arg : compillerArgs.clangArguments())
        ss << arg << ' ';

    return ss.str();
}

std::string generateReflectionCpp(const PersistentReflectionDB& reflectionDB) {
    std::stringstream ss;
    
    ss << "namespace flappy { class Reflection; }" << std::endl;
    ss << "using namespace flappy;" << std::endl;
    ss << std::endl;
    for (auto file : reflectionDB.reflectedFiles()) {
        if (filesystem::exists(file.cppFilePath))
            ss << "extern void " << file.functionName << "(flappy::Reflection&);" << std::endl;
    }
    ss << std::endl;
    ss << "bool generateReflection(flappy::Reflection& reflectionDB) {" << std::endl;
    
    for (auto file : reflectionDB.reflectedFiles()) {
        if (filesystem::exists(file.cppFilePath))
            ss << "    " << file.functionName << "(reflectionDB);" << std::endl;
    }
    
    ss << "    return true;" << std::endl;
    ss << "}" << std::endl;
    
    return ss.str();
}

int main(int argc, const char *argv[])
{
    GeneratorArgs generatorArgs(std::vector<std::string>{argv + 1, argv + argc});
    
    auto compillerArgs = parseCompillerArgs(generatorArgs.compillerPath(), generatorArgs.unrecognized());

    auto reflectionDBPath = generatorArgs.reflectionOutPath();
    reflectionDBPath.append("ReflectionDB.json");
    auto reflectionDB = PersistentReflectionDB(reflectionDBPath);

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
        ClassParser classParser([&generatedCppFiles](const TypeReflection& typeReflection,
                                                    const std::string& fileName,
                                                    const std::string& className)
        {
            auto originalFilePath = filesystem::absolute(fileName).normalize();
            filesystem::path newFilePath;
            newFilePath.append(originalFilePath.parent_path().string());
            newFilePath.append(originalFilePath.stem().string() + "_r" + originalFilePath.extension().string());
            newFilePath.normalize();
            
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
        
        reflectionDB.save();
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
    
    // Include path is needed for linking too, because I adding Reflection.cpp to link command.
    {
        auto reflectionIncludesPath = generatorArgs.reflectionIncludesPath();
        compillerArgs.addIncludePath(reflectionIncludesPath);
        reflectionIncludesPath.append("/../../Utility/src");
        compillerArgs.addIncludePath(reflectionIncludesPath);
    }

    return runClang(serializeCompillerArgs(generatorArgs.compillerPath(), compillerArgs));
}
