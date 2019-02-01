#include "CompilerInfo.hpp"

#include <iostream>
#include <regex>

#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>

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
            trim(line);
            
            std::cmatch m;
            
            std::regex re("version ([0-9]*?\\.[0-9]*?\\.[0-9]*)");
            
            if (std::regex_search(line.c_str(), m, re))
                m_version = m[1];
            
            const char installedDirMarker[] = "InstalledDir: ";
            if (line.find(installedDirMarker) == 0)
                m_clangInstallDir = line.substr(sizeof(installedDirMarker) / sizeof(installedDirMarker[0]) - 1);
        }
        
        if (m_version.empty())
            throw std::runtime_error("Can't get clang version by path " + compilerPath.string());
        
        if (m_clangInstallDir.empty())
            throw std::runtime_error("Can't access clang by path " + compilerPath.string());

        m_includeDirs.emplace_back(m_clangInstallDir + "/../include/c++/v1").normalize();
        m_includeDirs.emplace_back(m_clangInstallDir + "/../lib/clang/" + m_version + "/include").normalize();
    } catch (const std::exception& e) {
        std::cout << "Can't read installation dir and includes for compiller: " << compilerPath << std::endl;
        throw;
    }
}

