#include <cstdio>
#include <iostream>
#include <fstream>
#include <thread>
#include <stdexcept>
#include <unistd.h>
#include <sstream>
#include <unordered_set>
#include <regex>
#include <csignal>

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
#include "CompilerArgs.hpp"
#include "GeneratorArgs.hpp"
#include "PersistentReflectionDB.hpp"
#include "CompilerInfo.hpp"

using namespace boost;
using namespace std;
using namespace llvm;
using namespace clang::tooling;
using namespace clang;
using namespace clang::ast_matchers;

int runClang(const std::string& command)
{
    std::cout << "Running clang: " << command << std::endl;
    
    try {
        process::ipstream pipe_stream;
        process::child c(command, process::std_out > pipe_stream);

        std::string line;

        while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
            std::cout << line << std::endl;

        c.wait();
        return c.exit_code();
    } catch (const std::exception& e) {
        std::cout << "Can't run clang with command: " << command << std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }
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
    ss << "using namespace mojito;" << std::endl;
    ss << std::endl;
    ss << "void " << reflectionUnit.name << "(mojito::Reflection& reflection) {" << std::endl;
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

CompilerArgs parseCompillerArgs(const boost::filesystem::path& compillerPath, const std::vector<std::string>& arguments) {
    CompilerArgs compillerArgs;
    
    enum {
        INCLUDE_PATH,
        UNKNOWN,
        DEFINE,
        OUTPUT,
        XLINKER_OPTION,
        STANDARD,
        SKIP_ARG
    } state = UNKNOWN;
    
    for (const auto& arg : arguments) {
        if (state == UNKNOWN && (arg == "-MT" || arg == "-MF")) {
            state = SKIP_ARG;
            compillerArgs.addUnrecognizedArg(arg);
        } else if (state == UNKNOWN && arg.find("-I") == 0) {
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
        } else if (state == UNKNOWN && std::regex_match(arg, std::regex(".*?\\.cpp"))) {
            compillerArgs.addCppInputFile(arg);
            std::cout << "CPP: " << arg << std::endl;
        } else if (state == UNKNOWN && std::regex_match(arg, std::regex(".*?\\.o"))) {
            compillerArgs.addObjInputFile(arg);
            std::cout << "OBJ: " << arg << std::endl;
       } else if (state == UNKNOWN && std::regex_match(arg, std::regex(".*?\\.a"))) {
            compillerArgs.addLibInputFile(arg);
            std::cout << "LIB: " << arg << std::endl;
        } else if (state == INCLUDE_PATH) {
            std::cout << "INCLUDE_PATH: " << arg << std::endl;
            compillerArgs.addIncludePath(arg);
            state = UNKNOWN;
        } else if (state == DEFINE) {
            auto split = splitArgument(arg);
            std::cout << "DEFINE: " << arg << std::endl;
            compillerArgs.addDefine(std::get<0>(split), std::get<1>(split));
            state = UNKNOWN;
        } else if (state == OUTPUT) {
            std::cout << "OUTPUT: " << arg << std::endl;
            compillerArgs.setOutput(arg);
            state = UNKNOWN;
        } else if (state == XLINKER_OPTION) {
            std::cout << "XLINKER_OPTION: " << arg << std::endl;
            compillerArgs.addLinkerOption(arg);
            state = UNKNOWN;
        } else if (state == STANDARD) {
            compillerArgs.setCppStandard(arg);
            state = UNKNOWN;
        } else if (state == SKIP_ARG) {
            std::cout << "Skipped: " << arg << std::endl;
            compillerArgs.addUnrecognizedArg(arg);
            state = UNKNOWN;
        } else {
            std::cout << "UNRECOGNIZED: " << arg << std::endl;
            compillerArgs.addUnrecognizedArg(arg);
        }
    }
    
    return compillerArgs;
}

std::string serializeCompillerArgs(const filesystem::path& compillerPath, const CompilerArgs& compillerArgs) {
    std::stringstream ss;
    
    ss << compillerPath.string() << ' ';
    
    for (const auto& arg : compillerArgs.unrecognizedArgs())
        ss << arg << ' ';
    
    for (const auto& arg : compillerArgs.clangArguments())
        ss << arg << ' ';

    return ss.str();
}

std::string generateReflectionCpp(const PersistentReflectionDB& reflectionDB, const std::unordered_set<std::string>& outFiles) {
    std::stringstream ss;
    
    auto isPresented = [&outFiles](const PersistentReflectionDB::ReflectedFile& file){
        auto objName = file.outFilePath.filename().string();
        return filesystem::exists(file.cppFilePath) && outFiles.find(objName) != outFiles.end();
    };
    
    ss << "namespace mojito { class Reflection; }" << std::endl;
    ss << "using namespace mojito;" << std::endl;
    ss << std::endl;
    for (auto file : reflectionDB.reflectedFiles()) {
        if (isPresented(file))
            ss << "extern void " << file.functionName << "(mojito::Reflection&);" << std::endl;
    }
    ss << std::endl;
    ss << "bool generateReflection(Reflection& reflectionDB) {" << std::endl;
    
    for (auto file : reflectionDB.reflectedFiles()) {
        if (isPresented(file))
            ss << "    " << file.functionName << "(reflectionDB);" << std::endl;
    }
    
    ss << "    return true;" << std::endl;
    ss << "}" << std::endl;
    
    return ss.str();
}

std::unordered_set<std::string> listOfObjects(const GeneratorArgs&, const CompilerArgs& compillerArgs, const CompilerInfo& info) {
    if (compillerArgs.objInputFiles().empty() && compillerArgs.libInputFiles().empty())
        return {"unknown"};
    std::unordered_set<std::string> inputObjects;
    for (const auto& lib : compillerArgs.libInputFiles()) {
        auto arPath = info.clangInstallDir();
        arPath.append("ar");
        process::ipstream pipe_stream;
        process::child process(arPath.string() + " -t " + lib.string(), process::std_out > pipe_stream);
        
        std::cout << "AR command: " << (arPath.string() + " -t " + lib.string()) << std::endl;
        
        std::string line;
        while (process.running() && std::getline(pipe_stream, line) && !line.empty()) {
            if (std::regex_match(line, std::regex(".*?\\.o"))) {
                inputObjects.emplace(line);
                std::cout << "Object from lib: " << line << std::endl;
            }
        }
    }
    for (const auto& obj : compillerArgs.objInputFiles())
        inputObjects.emplace(obj.filename().string());
    return inputObjects;
}

int main(int argc, const char *argv[])
{

//    raise(SIGSTOP);
    GeneratorArgs generatorArgs(std::vector<std::string>{argv + 1, argv + argc});
    
    auto compillerArgs = parseCompillerArgs(generatorArgs.compillerPath(), generatorArgs.unrecognized());

    auto reflectionDBPath = generatorArgs.reflectionOutPath();
    reflectionDBPath.append(generatorArgs.reflectionName());
    reflectionDBPath.append("ReflectionDB.json");
    auto reflectionDB = PersistentReflectionDB(reflectionDBPath);

    CompilerInfo compilerInfo(generatorArgs.compillerPath());

    if (!compillerArgs.cppInputFiles().empty()) {
        std::cout << "Building sources" << std::endl;
    
        CustomCompilationDatabase compilationDatabase(compilerInfo, compillerArgs);

        std::vector<std::string> cppFiles;
        std::transform(compillerArgs.cppInputFiles().begin(), compillerArgs.cppInputFiles().end(), std::back_inserter(cppFiles),
            [](const auto& input) { return input.string(); });
        ClangTool tool(compilationDatabase, ArrayRef<std::string>(cppFiles));

        std::unordered_map<std::string, ReflectionUnit> generatedCppFiles;

        MatchFinder finder;
        auto methodMatcher = cxxRecordDecl(isClass(), isDefinition()).bind("classes");
        ClassParser classParser([&generatedCppFiles, &generatorArgs](const TypeReflection& typeReflection,
                                                    const std::string& fileName,
                                                    const std::string& className)
        {
            auto originalFilePath = filesystem::absolute(fileName).normalize();
            std::cout << "originalFilePath " << originalFilePath << std::endl;
            filesystem::path newFilePath = generatorArgs.reflectionOutPath();
            newFilePath.append(generatorArgs.reflectionName());
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
        
        // Initially injected cpp files are the same as original ones
        std::vector<filesystem::path> injectedCppFilePaths;
        std::transform(
            compillerArgs.cppInputFiles().begin(),
            compillerArgs.cppInputFiles().end(),
            std::back_inserter(injectedCppFilePaths),
            [] (const auto& input) {
                auto output = input.is_absolute() ? filesystem::path() : filesystem::current_path();
                output.append(input.string());
                output.normalize();
                return output;
            });

        // Replace files with reflection with generated files
        for (auto cppFile : generatedCppFiles) {
            auto found = std::find(injectedCppFilePaths.begin(), injectedCppFilePaths.end(), cppFile.second.originalPath);
            assert(found != injectedCppFilePaths.end());
            *found = cppFile.first;
            std::cout << "generated " << filesystem::path(cppFile.first).parent_path() << std::endl;
            filesystem::create_directories(filesystem::path(cppFile.first).parent_path());
            writeTextFile(cppFile.first, generateWrapperCpp(cppFile.second));
            reflectionDB.addReflectedFile({
                .cppFilePath = cppFile.second.originalPath,
                .reflectedCppFilePath = cppFile.first,
                .outFilePath = compillerArgs.output(),
                .functionName = cppFile.second.name
            });
        }
        
        compillerArgs.setCppInputFiles(injectedCppFilePaths);
        compillerArgs.addIncludePath(generatorArgs.reflectionIncludesPath());
        
        reflectionDB.save();
    }
    
    if (compillerArgs.output().extension() != ".o") {
        std::cout << "Linking" << std::endl;
        auto reflectionCppData = generateReflectionCpp(reflectionDB, listOfObjects(generatorArgs, compillerArgs, compilerInfo));
        auto reflectionCppPath = generatorArgs.reflectionOutPath();
        reflectionCppPath.append(generatorArgs.reflectionName());
        reflectionCppPath.append("Reflection.cpp");
        writeTextFile(reflectionCppPath, reflectionCppData);
        compillerArgs.addCppInputFile(reflectionCppPath);
        filesystem::create_directories(reflectionCppPath.parent_path());
    }

    return runClang(serializeCompillerArgs(generatorArgs.compillerPath(), compillerArgs));
}
