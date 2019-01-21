#include "ClassParser.hpp"

#include <iostream>
#include <unordered_set>
#include <sstream>

#include <clang/Frontend/FrontendActions.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Tooling.h>

using namespace clang::tooling;
using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;

namespace {

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string generateArgs(const CXXMethodDecl* methodDecl) {
    std::stringstream conditions;

    int index = 0;
    for (auto paramIter = methodDecl->param_begin(); paramIter != methodDecl->param_end(); ++paramIter) {
        auto qualType = (*paramIter)->getType();
        auto typeName = qualType.getAsString();

        replace(typeName, "_Bool", "bool");

        if (paramIter != methodDecl->param_begin())
            conditions << ", ";
        conditions << typeName;
        ++index;
    }

    return conditions.str();
}

std::string generateMethodBody(std::string methodName, std::string className, const std::vector<CXXMethodDecl*> methods) {
    std::stringstream methodConditionBlock;
    for (auto methodDecl : methods) {
        auto resultQualType = methodDecl->getReturnType();//.getNonReferenceType().getAtomicUnqualifiedType();
        auto resultTypeName = resultQualType.getAsString();
        replace(resultTypeName, "_Bool", "bool");
        auto args = generateArgs(methodDecl);

        std::cout << "    method " << methodName << "(" << args << ") -> " << resultTypeName << std::endl;
        //if (methodDecl->isConst()) {
        methodConditionBlock << ".addFunction(\"" << methodName << "\", &" << className << "::" << methodName << ")" << std::endl;
        //}
    }
    return methodConditionBlock.str();
}

std::string generateConstructorBody(std::string className, const std::vector<CXXMethodDecl*> methods, const CXXRecordDecl* classDecl) {
    std::stringstream constructorBody;
    if (!classDecl->isAbstract()) {
        for (auto methodDecl : methods) {
            if (methodDecl->isUserProvided())
                constructorBody << ".addConstructor<" << className << ", " << generateArgs(methodDecl) << ">()" << std::endl;
        }
        // default constructor
        if (classDecl->hasDefaultConstructor())
            constructorBody << ".addConstructor<" << className << ">()" << std::endl;
    }
    return constructorBody.str();
}

GeneratedMethods processMethods(const CXXRecordDecl* classDecl, const std::string& className) {
    std::stringstream methodBodies;

    std::unordered_map<std::string, std::vector<CXXMethodDecl*>> methods;

    for (auto iter = classDecl->method_begin(); iter != classDecl->method_end(); iter++) {
        const auto& methodName = iter->getNameAsString();
        if ((iter->isInstance() && iter->isUserProvided() && iter->getAccess() == AS_public && (methodName != "~" + className))
                || (methodName == className)) {
            methods[methodName].push_back(*iter);
        }
    }

    std::unordered_set<std::string> processedMethods;
    for (auto methodPair : methods) {
        auto overloadedMethods = methodPair.second;
        auto methodName = methodPair.first;
        std::cout << "    method " << methodName << std::endl;
        if (methodName == className) {
            methodBodies << generateConstructorBody(className, overloadedMethods, classDecl);
        } else {
            methodBodies << generateMethodBody(methodName, className, overloadedMethods);
        }
    }

    return GeneratedMethods{methodBodies.str()};
}
}

ClassParser::ClassParser(const Callback& callback)
    : m_callback(callback)
{}

void ClassParser::run(const MatchFinder::MatchResult &result) {
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

        auto fileName = result.SourceManager->getFilename(classDecl->getLocation());

        auto typeReflection = TypeReflection{ className, processMethods(classDecl, className) };

        m_callback(typeReflection, fileName, className);
    }
}
