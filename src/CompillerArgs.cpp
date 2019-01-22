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
    for (const auto& linkerOption : m_linkerOptions) {
        arguments.emplace_back("-Xlinker ");
        arguments.emplace_back(linkerOption);
    }
    arguments.emplace_back("-std=" + m_cppStandard);

    return arguments;
}
