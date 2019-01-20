#pragma once

#include <functional>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

class ClassParser : public clang::ast_matchers::MatchFinder::MatchCallback {
public :
    using Callback = std::function<void(const clang::CXXRecordDecl*, const std::string&, const std::string&)>;

    ClassParser(const Callback& callback);

    void run(const clang::ast_matchers::MatchFinder::MatchResult &result);

private:
    Callback m_callback;
};
