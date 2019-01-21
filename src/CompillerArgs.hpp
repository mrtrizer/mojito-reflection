#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <boost/filesystem.hpp>

class CompillerArgs {
public:
    using FilePathList = std::vector<boost::filesystem::path>;

    const FilePathList& cppInputFiles() const { return m_cppInputFiles; }
    void setCppInputFiles(const FilePathList& cppInputFiles) { m_cppInputFiles = cppInputFiles; }
    void addCppInputFile(const boost::filesystem::path& path) { m_cppInputFiles.emplace_back(path); }
    
    const FilePathList& objInputFiles() const { return m_objInputFiles; }
    void setObjInputFiles(const FilePathList& objInputFiles) { m_objInputFiles = objInputFiles; }
    void addObjInputFile(const boost::filesystem::path& path) { m_objInputFiles.emplace_back(path); }
    
    void addIncludePath(const boost::filesystem::path& path) { m_includePathes.emplace_back(path); }
    
    void addDefine(const std::string& define, const std::string& value) { m_defines.emplace(define, value); }
    
    void setOutput(const boost::filesystem::path& output) { m_output = output; }
    const boost::filesystem::path& output() const { return m_output; }
    
    void addLinkerOption(const std::string& option) { m_linkerOptions.emplace_back(option); }
    
    void addUnrecognizedArg(const std::string& unrecognized) { m_unrecognizedArgs.emplace_back(unrecognized); }
    const std::vector<std::string> unrecognizedArgs() { return m_unrecognizedArgs; }
    
    std::vector<std::string> clangArguments() const;
    
    std::vector<std::string> allArguments() const;
    
    std::string serialize() const;
    
private:
    FilePathList m_objInputFiles;
    FilePathList m_cppInputFiles;
    FilePathList m_includePathes;
    std::unordered_map<std::string, std::string> m_defines;
    boost::filesystem::path m_output;
    std::vector<std::string> m_linkerOptions;
    std::vector<std::string> m_unrecognizedArgs;
};
