#include "CompilerArgs.hpp"

#include <sstream>
#include <iostream>

void CompilerArgs::addCppInputFile(const boost::filesystem::path& path) {
    m_cppInputFiles.emplace_back(boost::filesystem::canonical(path));
}

void CompilerArgs::addObjInputFile(const boost::filesystem::path& path) {
    m_objInputFiles.emplace_back(boost::filesystem::canonical(path));
}

void CompilerArgs::addLibInputFile(const boost::filesystem::path& path) {
    m_libInputFiles.emplace_back(boost::filesystem::canonical(path));
}

void CompilerArgs::addIncludePath(const boost::filesystem::path& path) {
    try {
        m_includePathes.emplace_back(boost::filesystem::canonical(path));
    } catch (const std::exception& e) {
        std::cout << "Can't add include path: " << e.what();
    }
}

void CompilerArgs::setOutput(const boost::filesystem::path& output) {
    m_output = boost::filesystem::absolute(output);
    m_output.normalize();
}

std::vector<std::string> CompilerArgs::clangArguments() const {
    std::vector<std::string> arguments;
    
    for (const auto& filePath : m_objInputFiles)
        arguments.emplace_back(filePath.string());
    for (const auto& filePath : m_cppInputFiles)
        arguments.emplace_back(filePath.string());
    for (const auto& filePath : m_libInputFiles)
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
    if (!m_cppStandard.empty())
        arguments.emplace_back("-std=" + m_cppStandard);

    return arguments;
}
