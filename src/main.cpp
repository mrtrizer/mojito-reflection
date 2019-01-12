#include <cstdio>
#include <iostream>
#include <fstream>
#include <thread>
#include <stdexcept>
#include <unistd.h>
#include <sstream>
#include <unordered_set>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/Support/CommandLine.h>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>

#include <boost/process.hpp>

#include <boost/filesystem.hpp>

#include "generate_method.h"

using namespace boost;
using namespace boost::filesystem;
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
                                const path& originalFilePath) {
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

void writeTextFile(const path& filePath, const std::string& data) {
    std::ofstream textFile;
    textFile.open(filePath.string());
    textFile << data;
    textFile.close();
}


class ClassHandler : public MatchFinder::MatchCallback {
public :
    using Callback = std::function<void(const CXXRecordDecl*, const std::string&)>;

    ClassHandler(const Callback& callback)
        : m_callback(callback)
    {}

    virtual void run(const MatchFinder::MatchResult &result) {
        if (const CXXRecordDecl *classDecl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("classes")) {
        
            auto className = classDecl->getNameAsString();
        
            if(!classDecl->hasAttrs())
                return;
            
            clang::AttrVec vec = classDecl->getAttrs();

            if (strcmp(vec[0]->getSpelling(), "annotate") != 0)
                return;
            
            
            SourceLocation startLoc = vec[0]->getRange().getBegin();
            SourceLocation endLoc = vec[0]->getRange().getEnd();
  
            std::cout << "isMacroID: " << startLoc.isMacroID() << std::endl;

            if( startLoc.isMacroID() ) {
                startLoc = result.SourceManager->getImmediateSpellingLoc( startLoc );
                endLoc = result.SourceManager->getImmediateSpellingLoc( endLoc );
            }
            
            auto tokenRange = CharSourceRange::getCharRange(startLoc, endLoc);
            auto annotateValue = Lexer::getSourceText(tokenRange, *result.SourceManager, result.Context->getLangOpts()).str();
            std::cout << className << " : " << vec[0]->getSpelling() << " : " << annotateValue << std::endl;
            if (annotateValue != "annotate(\"reflect\"")
                return;
        
            if (classDecl->getDescribedClassTemplate() != nullptr) {
                std::cout << "Skip template class " << classDecl->getNameAsString() << std::endl;
                return;
            }
            if (classDecl->getDeclKind() == clang::Decl::ClassTemplatePartialSpecialization) {
                std::cout << "Skip template partial specialization " << classDecl->getNameAsString() << std::endl;
                return;
            }
            if (classDecl->getDeclKind() == clang::Decl::ClassTemplateSpecialization) {
                std::cout << "Skip template specialization " << classDecl->getNameAsString() << std::endl;
                return;
            }

            std::cout << "class " << className << std::endl;

            m_callback(classDecl, className);
        }
    }

private:
    Callback m_callback;
};

class CustomCompilationDatabase : public CompilationDatabase {
public:
    CustomCompilationDatabase(std::string compillerCommand, const path& cppPath, const std::multimap<std::string, std::string>& args)
        : m_cppPath(cppPath)
    {
        m_args.emplace_back(compillerCommand);
        for (auto arg : args) {
            if (arg.first.empty())
                m_args.emplace_back(cppPath.string());
            else {
                m_args.emplace_back(arg.first);
                if (!arg.second.empty())
                    m_args.emplace_back(arg.second);
            }
        }
    }

    virtual std::vector<CompileCommand> getCompileCommands(StringRef filePath) const override {
        return getAllCompileCommands();
    }

    virtual std::vector<std::string> getAllFiles() const override {
        return { m_cppPath.filename().string() };
    }

    virtual std::vector<CompileCommand> getAllCompileCommands() const override {
        return { CompileCommand { current_path().string(), m_cppPath.filename().string(), m_args, "" } };
    }
    
private:
    path m_cppPath;
    std::vector<std::string> m_args;
};

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
        throw std::runtime_error("Set compiller path with --cpp-path parameter");
    
    auto fileNameIter = args.find("");
    if (fileNameIter == args.end())
        throw std::runtime_error("No source file");
    
    auto reflectionPathIter = args.find(reflectionDir);
    if (reflectionPathIter == args.end())
        throw std::runtime_error("No reflection includes path use --reflection /path/to/reflection");

    std::string fileName = fileNameIter->second;
    path fileAbsPath = current_path();
    fileAbsPath.append(fileName);
    fileAbsPath.normalize();
    path newFileAbsPath = current_path();
    newFileAbsPath.append("r_" + fileName);
    newFileAbsPath.normalize();

    auto outputArgs = clearArgs(args);


    CustomCompilationDatabase compilationDatabase(compileCommandIter->second, fileAbsPath, outputArgs);

    ClangTool tool(compilationDatabase, ArrayRef<std::string>(&fileName, 1));
    
    MatchFinder finder;
    auto methodMatcher = cxxRecordDecl(isClass(), isDefinition()).bind("classes");
    ClassHandler printer([&fileAbsPath, &newFileAbsPath](const CXXRecordDecl* classDecl, const std::string& className) {
        auto generatedMethods = processMethods(classDecl, className);
        auto cppFileData = generateWrapperCpp(className, generatedMethods, fileAbsPath);
        auto cppFileName = fileAbsPath.filename();
        writeTextFile(newFileAbsPath, cppFileData);
    });
    finder.addMatcher(methodMatcher, &printer);
    
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
