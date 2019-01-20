#include "ClassParser.hpp"

#include <iostream>

#include <clang/Tooling/Tooling.h>

using namespace clang::tooling;
using namespace clang;
using namespace clang::ast_matchers;

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

        m_callback(classDecl, fileName, className);
    }
}
