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

int runClang(int argc, const char* argv[])
{
    stringstream ss;
    ss << "clang ";
    for (int i = 1; i < argc; ++i)
        ss << argv[i] << " ";
    
    Process process(ss.str(), currentDir(), [](const char *bytes, size_t n) {
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
    CustomCompilationDatabase(std::string dir, int argc, const char** argv) : m_dir(dir) {
    
    }

    virtual std::vector<CompileCommand> getCompileCommands(StringRef filePath) const override {
        return getAllCompileCommands();
    }

    virtual std::vector<std::string> getAllFiles() const override {
        return {"test.cpp"};
    }

    virtual std::vector<CompileCommand> getAllCompileCommands() const override {
        return { CompileCommand { m_dir, "test.cpp", {"test.cpp"}, "test.o" } };
    }
    
private:
    std::string m_dir;
};

int main(int argc, const char *argv[])
{
    CommonOptionsParser optionParser(argc, argv, llvm::cl::GeneralCategory);

   // CustomCompilationDatabase compilationDatabase(currentDir(), argc, argv);

    std::string fileName = argv[1];

    ClangTool tool(optionParser.getCompilations(), ArrayRef<std::string>(&fileName, 1));
    
    MatchFinder finder;
    auto methodMatcher = cxxRecordDecl(isClass(), isDefinition()).bind("classes");
    ClassHandler printer(currentDir());
    finder.addMatcher(methodMatcher, &printer);
    
    tool.run(newFrontendActionFactory(&finder).get());

    return runClang(argc, argv);
}
