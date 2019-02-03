#include "CompilerInfo.hpp"

#include <iostream>
#include <regex>

#include <boost/process.hpp>

CompilerInfo::CompilerInfo(const boost::filesystem::path& compilerPath)
    : m_compilerPath(compilerPath)
{
    using namespace boost;

    try {
        process::ipstream pipe_stream;
        
        std::string command = compilerPath.string() + " -v";
        
        process::child c(command, process::std_err > pipe_stream);

        std::string line;
        while (!pipe_stream.eof() && std::getline(pipe_stream, line) && !line.empty()) {
            
            std::cmatch m;
            
            if (std::regex_search(line.c_str(), m, std::regex("version ([0-9]*?\\.[0-9]*?\\.[0-9]*)")))
                m_version = m[1];
            
            if (std::regex_search(line.c_str(), m, std::regex("InstalledDir: (.*)")))
                m_clangInstallDir = m[1];
        }
        
        if (m_version.empty())
            throw std::runtime_error("Can't get clang version by path " + compilerPath.string());
        
        if (m_clangInstallDir.empty())
            throw std::runtime_error("Can't access clang by path " + compilerPath.string());

        try {
            auto llvmIncludesDir = m_clangInstallDir;
            llvmIncludesDir.append("/../include/c++/v1");
            m_includeDirs.emplace_back(boost::filesystem::canonical(llvmIncludesDir));
        } catch (const std::exception& e){
            std::cout << "Can't find LLVM includes dir " << e.what();
        }
        
        try {
            auto clangIncludesDir = m_clangInstallDir;
            clangIncludesDir.append("../lib/clang/");
            clangIncludesDir.append(m_version);
            clangIncludesDir.append("include");
            m_includeDirs.emplace_back(boost::filesystem::canonical(clangIncludesDir));
        } catch (const std::exception& e){
            std::cout << "Can't find Clang includes dir " << e.what();
        }
    } catch (const std::exception& e) {
        std::cout << "Can't read installation dir and includes for compiller: " << compilerPath << std::endl;
        std::cout << e.what() << std::endl;
        throw;
    }
}

