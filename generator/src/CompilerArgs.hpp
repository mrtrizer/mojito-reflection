#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <boost/filesystem.hpp>

class CompilerArgs {
public:
    using FilePathList = std::vector<boost::filesystem::path>;

    const FilePathList& cppInputFiles() const { return m_cppInputFiles; }
    void setCppInputFiles(const FilePathList& cppInputFiles) { m_cppInputFiles = cppInputFiles; }
    void addCppInputFile(const boost::filesystem::path& path);
    
    const FilePathList& objInputFiles() const { return m_objInputFiles; }
    void setObjInputFiles(const FilePathList& objInputFiles) { m_objInputFiles = objInputFiles; }
    void addObjInputFile(const boost::filesystem::path& path);

    const FilePathList& libInputFiles() const { return m_libInputFiles; }
    void setLibInputFiles(const FilePathList& libInputFiles) { m_libInputFiles = libInputFiles; }
    void addLibInputFile(const boost::filesystem::path& path);

    void addIncludePath(const boost::filesystem::path& path);
    
    void addDefine(const std::string& define, const std::string& value) { m_defines.emplace(define, value); }
    
    void setOutput(const boost::filesystem::path& output);
    const boost::filesystem::path& output() const { return m_output; }
    
    void addLinkerOption(const std::string& option) { m_linkerOptions.emplace_back(option); }
    
    void addUnrecognizedArg(const std::string& unrecognized) { m_unrecognizedArgs.emplace_back(unrecognized); }
    const std::vector<std::string> unrecognizedArgs() const { return m_unrecognizedArgs; }
    
    void setCppStandard(const std::string& standard) { m_cppStandard = standard; }
    const std::string& cppStandard() const { return m_cppStandard; }
    
    std::vector<std::string> clangArguments() const;
    
private:
    FilePathList m_objInputFiles;
    FilePathList m_cppInputFiles;
    FilePathList m_libInputFiles;
    FilePathList m_includePathes;
    std::unordered_map<std::string, std::string> m_defines;
    boost::filesystem::path m_output = "unknown";
    std::vector<std::string> m_linkerOptions;
    std::vector<std::string> m_unrecognizedArgs;
    std::string m_cppStandard;
};
