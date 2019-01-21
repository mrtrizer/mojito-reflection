#include "CompillerArgs.hpp"

#include <sstream>

std::vector<std::string> CompillerArgs::clangArguments() const {
    std::vector<std::string> arguments;
    
    for (const auto& filePath : m_objInputFiles)
        arguments.emplace_back(filePath.string());
    for (const auto& filePath : m_cppInputFiles)
        arguments.emplace_back(filePath.string());
    for (const auto& filePath : m_includePathes) {
        arguments.emplace_back("-I");
        arguments.emplace_back(filePath.string());
    }
    for (const auto& define : m_defines) {
        arguments.emplace_back("-D");
        arguments.emplace_back(define.first + (define.second.empty() ? "" : "=" + define.second));
    }
    if (!m_output.empty()) {
        arguments.emplace_back("-o");
        arguments.emplace_back(m_output.string());
    }
    
    return arguments;
}

std::vector<std::string> CompillerArgs::allArguments() const {
    std::vector<std::string> arguments;
    
    for (const auto& unrecognized : m_unrecognizedArgs)
        arguments.emplace_back(unrecognized);
    for (const auto& filePath : m_objInputFiles)
        arguments.emplace_back(filePath.string());
    for (const auto& filePath : m_cppInputFiles)
        arguments.emplace_back(filePath.string());
    for (const auto& filePath : m_includePathes) {
        arguments.emplace_back("-I");
        arguments.emplace_back(filePath.string());
    }
    for (const auto& define : m_defines) {
        arguments.emplace_back("-D");
        arguments.emplace_back(define.first + (define.second.empty() ? "" : "=" + define.second));
    }
    if (!m_output.empty()) {
        arguments.emplace_back("-o");
        arguments.emplace_back(m_output.string());
    }
    for (const auto& linkerOption : m_linkerOptions) {
        arguments.emplace_back("-Xlinker ");
        arguments.emplace_back(linkerOption);
    }
    
    return arguments;
}


std::string CompillerArgs::serialize() const {
    std::stringstream ss;
    
    ss << "clang++ ";
    
    for (const auto& arg : allArguments())
        ss << arg << ' ';

    return ss.str();
}
