#pragma once

#include <boost/filesystem.hpp>
#include <vector>
#include <string>

class GeneratorArgs {
public:
    GeneratorArgs(const std::vector<std::string>& args);
    
    const boost::filesystem::path& compillerPath() const noexcept { return m_compillerPath; }
    const boost::filesystem::path& reflectionIncludesPath() const noexcept { return m_reflectionIncludesPath; }
    const boost::filesystem::path& reflectionOutPath() const noexcept { return m_reflectionOutPath; }
    
    const std::vector<std::string>& unrecognized() const noexcept { return m_unrecognized; }
    
private:
    boost::filesystem::path m_compillerPath;
    boost::filesystem::path m_reflectionIncludesPath;
    boost::filesystem::path m_reflectionOutPath;
    std::vector<std::string> m_unrecognized;
};
