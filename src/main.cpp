#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include <clang/Tooling/CompilationDatabase.h>
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/Support/CommandLine.h"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/ADT/StringRef.h"

#include "generate_method.h"
#include "json.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <thread>
#include <stdexcept>
#include <unistd.h>
#include <sstream>
#include <unordered_set>

#include <process.hpp>
#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif

using namespace std;
using namespace TinyProcessLib;
using namespace llvm;
using namespace clang::tooling;
using namespace clang;
using namespace clang::ast_matchers;

std::string create(std::string directory, std::string command, std::string file) {
    stringstream ss;
    ss << "[ {\n \"directory\": \"" << directory << "\",\n \"command\": \"" << command << "\",\n \"file\": \"" << file << " \" \n} ]";
    return ss.str();
}

std::string currentDir() {
    char currentDir[FILENAME_MAX];
    
    if (!GetCurrentDir(currentDir, sizeof(currentDir)))
        throw std::runtime_error("Can't get current dir");
    
    return currentDir;
}

int runClang(std::string command)
{
    Process process(command, currentDir(), [](const char *bytes, size_t n) {
        std::cout << std::string(bytes, n);
    }, [](const char *bytes, size_t n) {
        std::cout << std::string(bytes, n);
    });
    
    return process.get_exit_status();
}

class ClassHandler : public MatchFinder::MatchCallback {
public :
    ClassHandler(std::string path)
        : m_path(path)
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
            if (m_wrappedClasses.find(className) != m_wrappedClasses.end())
                return;

            m_wrappedClasses.insert(className);

            std::cout << "class " << className << std::endl;

            auto generatedMethods = processMethods(classDecl, className);

            //auto cppFileData = generateWrapperCpp(className, generatedMethods);
            //auto cppFileName = std::string("RR_") + className + ".cpp";
            //writeTextFile(m_path + "/" + cppFileName, cppFileData);

            //m_sourcesToGeneratedMap.emplace(cppFileName, result.SourceManager->getFilename(classDecl->getLocation()).str());
        }
    }

    const std::map<std::string, std::string>& sourcesToGeneratedMap() const {
        return m_sourcesToGeneratedMap;
    }
    
    const std::unordered_set<std::string>& wrappedClasses() const {
        return m_wrappedClasses;
    }

private:
    std::string m_path;
    std::unordered_set<std::string> m_wrappedClasses;
    std::map<std::string, std::string> m_sourcesToGeneratedMap;
};

class CustomCompilationDatabase : public CompilationDatabase {
public:
    CustomCompilationDatabase(std::string dir, std::string cppPath, int argc, const char** argv)
        : m_dir(dir)
        , m_cppPath(cppPath)
    {
        m_args.emplace_back(cppPath);
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.find("cpp-path") != std::string::npos)
                continue;
            if (arg[0] != '-')
                m_fileName = arg;
            m_args.emplace_back(std::move(arg));
        }
    }

    virtual std::vector<CompileCommand> getCompileCommands(StringRef filePath) const override {
        return getAllCompileCommands();
    }

    virtual std::vector<std::string> getAllFiles() const override {
        return {m_fileName};
    }

    virtual std::vector<CompileCommand> getAllCompileCommands() const override {
        return { CompileCommand { m_dir, m_fileName, m_args, "" } };
    }
    
private:
    std::string m_dir;
    std::string m_cppPath;
    std::vector<std::string> m_args;
    std::string m_fileName;
};

std::multimap<std::string, std::string> parseArgs(int argc, const char *argv[]) {
    std::multimap<std::string, std::string> args;

    auto currentArg = args.end();
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            auto separatorPos = arg.find('=');
            currentArg = args.emplace(arg.substr(0, separatorPos), arg.substr(separatorPos + 1, arg.size()));
            if (separatorPos == std::string::npos)
                currentArg = args.end();
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

const char* cppPathArg = "--cpp-path";

std::multimap<std::string, std::string> clearArgs(const std::multimap<std::string, std::string>& args) {
    auto tmp = args;
    for( auto it = tmp.begin(); it != tmp.end(); ) {
        if (it->first == cppPathArg)
            it = tmp.erase(it);
        else
            ++it;
    }
    return tmp;
}

int main(int argc, const char *argv[])
{
    auto args = parseArgs(argc, argv);
    
    auto cppPathIter = args.find(cppPathArg);
    if (cppPathIter == args.end())
        throw std::runtime_error("Set compiller path with --cpp-path parameter");
    
    auto fileNameIter = args.find("");
    if (fileNameIter == args.end())
        throw std::runtime_error("No source file");

    CustomCompilationDatabase compilationDatabase(currentDir(), cppPathIter->second, argc, argv);

    std::string fileName = fileNameIter->second;

    ClangTool tool(compilationDatabase, ArrayRef<std::string>(&fileName, 1));
    
    MatchFinder finder;
    auto methodMatcher = cxxRecordDecl(isClass(), isDefinition()).bind("classes");
    ClassHandler printer(currentDir());
    finder.addMatcher(methodMatcher, &printer);
    
    tool.run(newFrontendActionFactory(&finder).get());

    return runClang(concatArgs(cppPathIter->second, clearArgs(args)));
}
