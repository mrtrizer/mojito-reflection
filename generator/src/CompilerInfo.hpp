#pragma once

#include <vector>
#include <string>

#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>

class CompilerInfo {
public:
    CompilerInfo(const boost::filesystem::path& compilerPath);
    
    const boost::filesystem::path& compillerPath() const {
        return m_compilerPath;
    }
    
    const std::vector<boost::filesystem::path>& includeDirs() const {
        return m_includeDirs;
    }
    
    const boost::filesystem::path& clangInstallDir() const {
        return m_clangInstallDir;
    }
    
    const std::string& version() const {
        return m_version;
    }
    
private:
    boost::filesystem::path m_compilerPath;
    std::vector<boost::filesystem::path> m_includeDirs;
    boost::filesystem::path m_clangInstallDir;
    std::string m_version;
};
