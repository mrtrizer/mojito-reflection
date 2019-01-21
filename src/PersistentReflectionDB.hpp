#pragma once

#include <string>
#include <vector>
#include <boost/filesystem.hpp>

class PersistentReflectionDB {
public:
    PersistentReflectionDB(const boost::filesystem::path& reflectionDbFilePath);
    
    struct ReflectedFile {
        boost::filesystem::path cppFilePath;
        boost::filesystem::path reflectedCppFilePath;
        boost::filesystem::path outFilePath;
        std::string functionName;
    };
    
    const std::vector<ReflectedFile>& reflectedFiles() const { return m_reflectedFiles; }
    
    using FilePath = boost::filesystem::path;
    
    void addReflectedFile(const FilePath& cppFilePath, const FilePath& reflectedCppFilePath, const std::string& funcName);
    
    void save();
    
private:
    boost::filesystem::path m_reflectionDbFilePath;
    std::vector<ReflectedFile> m_reflectedFiles;
    
    inline static const char* reflectedFilesKey = "reflected_files";
    inline static const char* cppFilePathKey = "cpp_file_path";
    inline static const char* outFilePathKey = "out_file_path";
    inline static const char* functionNameKey = "function_name";
};
