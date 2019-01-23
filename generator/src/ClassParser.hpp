#pragma once

#include <functional>

#include <clang/ASTMatchers/ASTMatchFinder.h>

struct GeneratedMethods {
    std::string methodBodies;
};

struct TypeReflection {
    std::string typeName;
    GeneratedMethods methods;
};

class ClassParser : public clang::ast_matchers::MatchFinder::MatchCallback {
public :
    using Callback = std::function<void(const TypeReflection&, const std::string&, const std::string&)>;

    ClassParser(const Callback& callback);

    void run(const clang::ast_matchers::MatchFinder::MatchResult &result);

private:
    Callback m_callback;
};
