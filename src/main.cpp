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

#include "generate_method.h"
#include "CustomCompilationDB.hpp"
#include "ClassParser.hpp"

using namespace boost;
using namespace std;
using namespace llvm;
using namespace clang::tooling;
using namespace clang;
using namespace clang::ast_matchers;

int runClang(const std::string& command)
{
    process::ipstream pipe_stream;
    process::child c(command, process::std_out > pipe_stream);

    std::string line;

    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
        std::cout << line << std::endl;

    c.wait();
    return c.exit_code();
}

std::string generateWrapperCpp(const std::string& className,
                                const GeneratedMethods& generatedMethods,
                                const filesystem::path& originalFilePath) {
    std::vector<char> output(50000);
    snprintf(output.data(), output.size(),
            "#include <%s>\n"
            "#include <Type.hpp>\n"
            "#include <BasicTypesReflection.hpp>\n"
            "\n"
            "namespace flappy {\n"
            "void register%s(Reflection& reflection) {\n"
             "  reflection.registerType<%s>(\"%s\")\n"
             "%s;\n"
            "} \n"
            "} \n"
            "\n"
            "",
            originalFilePath.c_str(),
            className.c_str(),
            className.c_str(),
            className.c_str(),
            generatedMethods.methodBodies.c_str());
    return std::string(output.data());
}

void writeTextFile(const filesystem::path& filePath, const std::string& data) {
    std::ofstream textFile;
    textFile.open(filePath.string());
    textFile << data;
    textFile.close();
}



std::multimap<std::string, std::string> parseArgs(int argc, const char *argv[]) {
    std::multimap<std::string, std::string> args;

    auto currentArg = args.end();
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            currentArg = args.emplace(arg, "");
        } else {
            if (currentArg != args.end()) {
                currentArg->second = arg;
                currentArg = args.end();
            } else {
                args.emplace("", arg);
            }
        }
    }
    
    return args;
}

std::string concatArgs(std::string exec, std::multimap<std::string, std::string> args) {
    stringstream ss;
    ss << exec;
    for (auto arg : args) {
        ss << arg.first << " " << arg.second << " ";
    }
    return ss.str();
}

const char* compileCommand = "--compile-command";
const char* reflectionDir = "--reflection-dir";
const char* reflectionOut = "--reflection-out";
const char* reflectionName = "--reflection-name";

std::multimap<std::string, std::string> clearArgs(const std::multimap<std::string, std::string>& args) {
    auto tmp = args;
    for( auto it = tmp.begin(); it != tmp.end(); ) {
        if (it->first == compileCommand
            || it->first == reflectionDir
            || it->first == reflectionOut
            || it->first == reflectionName)
            it = tmp.erase(it);
        else
            ++it;
    }
    return tmp;
}

int main(int argc, const char *argv[])
{
    auto args = parseArgs(argc, argv);
    
    auto compileCommandIter = args.find(compileCommand);
    if (compileCommandIter == args.end())
        throw std::runtime_error("Set compiller filesystem::path with --cpp-filesystem::path parameter");
    
    auto fileNameIter = args.find("");
    if (fileNameIter == args.end())
        throw std::runtime_error("No source file");
    
    auto reflectionPathIter = args.find(reflectionDir);
    if (reflectionPathIter == args.end())
        throw std::runtime_error("No reflection includes filesystem::path use --reflection /filesystem::path/to/reflection");

    std::string fileName = fileNameIter->second;
    filesystem::path fileAbsPath = filesystem::current_path();
    fileAbsPath.append(fileName);
    fileAbsPath.normalize();
    filesystem::path newFileAbsPath = filesystem::current_path();
    newFileAbsPath.append("r_" + fileName);
    newFileAbsPath.normalize();

    auto outputArgs = clearArgs(args);


    CustomCompilationDatabase compilationDatabase(compileCommandIter->second, fileAbsPath, outputArgs);

    ClangTool tool(compilationDatabase, ArrayRef<std::string>(&fileName, 1));
    
    MatchFinder finder;
    auto methodMatcher = cxxRecordDecl(isClass(), isDefinition()).bind("classes");
    ClassParser classParser([&fileAbsPath, &newFileAbsPath](const CXXRecordDecl* classDecl, const std::string& className) {
        auto generatedMethods = processMethods(classDecl, className);
        auto cppFileData = generateWrapperCpp(className, generatedMethods, fileAbsPath);
        auto cppFileName = fileAbsPath.filename();
        writeTextFile(newFileAbsPath, cppFileData);
    });
    finder.addMatcher(methodMatcher, &classParser);
    
    tool.run(newFrontendActionFactory(&finder).get());
    
    outputArgs.emplace("-I", reflectionPathIter->second);
    outputArgs.emplace("-I", reflectionPathIter->second + "/../../Utility/src");
    auto sourceFileIter = outputArgs.find("");
    if (sourceFileIter == outputArgs.end())
        throw std::runtime_error("No source file in args");
    sourceFileIter->second = newFileAbsPath.string();

    auto compillerFullCommand = concatArgs(compileCommandIter->second, outputArgs);
    std::cout << compillerFullCommand << std::endl;

    return runClang(compillerFullCommand);
}
