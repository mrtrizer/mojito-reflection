#include "PersistentReflectionDB.hpp"

#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

PersistentReflectionDB::PersistentReflectionDB(const boost::filesystem::path& reflectionDbFilePath)
    : m_reflectionDbFilePath(reflectionDbFilePath)
{
    using namespace boost::property_tree;

    std::ifstream ifs (reflectionDbFilePath.string());
    
    if (!ifs.eof() && !ifs.fail()) {
        ptree pt;

        read_json(ifs, pt);
        
        for (const auto& value : pt.get_child(reflectedFilesKey)) {
            auto reflectedFileData = value.second;
            ReflectedFile reflectedFile;
            reflectedFile.cppFilePath = reflectedFileData.get_child(cppFilePathKey).get_value<std::string>();
            reflectedFile.outFilePath = reflectedFileData.get_child(outFilePathKey).get_value<std::string>();
            reflectedFile.functionName = reflectedFileData.get_child(functionNameKey).get_value<std::string>();
            m_reflectedFiles.emplace_back(reflectedFile);
        }
    }
}

void PersistentReflectionDB::addReflectedFile(const FilePath& cppFilePath, const FilePath& reflectedCppFilePath, const std::string& funcName) {
    auto iter = std::find_if(
        m_reflectedFiles.begin(),
        m_reflectedFiles.end(),
        [&cppFilePath](const auto& item) { return item.cppFilePath == cppFilePath; });
    auto reflectedFile = [&]()-> ReflectedFile { return {cppFilePath, reflectedCppFilePath, "", funcName}; };
    if (iter == m_reflectedFiles.end())
        m_reflectedFiles.emplace_back(reflectedFile());
    else
        *iter = reflectedFile();
}

void PersistentReflectionDB::save() {
    using namespace boost::property_tree;
    
    ptree pt;
    
    ptree subtree;
    
    for (const auto& reflectedFile : m_reflectedFiles) {
        ptree reflectedFileData;
        reflectedFileData.add(cppFilePathKey, reflectedFile.cppFilePath.string());
        reflectedFileData.add(outFilePathKey, reflectedFile.outFilePath.string());
        reflectedFileData.add(functionNameKey, reflectedFile.functionName);
        subtree.push_back(std::make_pair("", reflectedFileData));
    }
    
    pt.add_child(reflectedFilesKey, subtree);
    
    write_json(m_reflectionDbFilePath.string(), pt);
}
