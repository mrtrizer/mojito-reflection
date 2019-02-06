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
    
    std::vector<ReflectedFile> reflectedFiles() const;
    
    using FilePath = boost::filesystem::path;
    
    void addReflectedFile(const ReflectedFile& reflectedFile);

private:
    boost::filesystem::path m_reflectionDbFilePath;
};
