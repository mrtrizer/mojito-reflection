#include "PersistentReflectionDB.hpp"

#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

const char* reflectedFilesKey = "reflected_files";
const char* cppFilePathKey = "cpp_file_path";
const char* reflectedCppFilePathKey = "reflected_cpp_file_path";
const char* outFilePathKey = "out_file_path";
const char* functionNameKey = "function_name";
const char* jsonExtension = ".json";

PersistentReflectionDB::PersistentReflectionDB(const boost::filesystem::path& reflectionDbFilePath)
    : m_reflectionDbFilePath(reflectionDbFilePath)
{
    using namespace boost::filesystem;
    if (exists(reflectionDbFilePath) && !is_directory(reflectionDbFilePath))
        throw std::runtime_error("Reflection path should be dir: " + reflectionDbFilePath.string());
}

static PersistentReflectionDB::ReflectedFile readReflectedFile(boost::filesystem::path path) {
    using namespace boost;

    property_tree::ptree reflectedFileData;
    property_tree::read_json(path.string(), reflectedFileData);

    PersistentReflectionDB::ReflectedFile reflectedFile;
    reflectedFile.cppFilePath = reflectedFileData.get_child(cppFilePathKey).get_value<std::string>();
    reflectedFile.reflectedCppFilePath = reflectedFileData.get_child(reflectedCppFilePathKey).get_value<std::string>();
    reflectedFile.outFilePath = reflectedFileData.get_child(outFilePathKey).get_value<std::string>();
    reflectedFile.functionName = reflectedFileData.get_child(functionNameKey).get_value<std::string>();
    
    return reflectedFile;
}

std::vector<PersistentReflectionDB::ReflectedFile> PersistentReflectionDB::reflectedFiles() const
{
    using namespace boost;
    
    if (!filesystem::exists(m_reflectionDbFilePath))
        return {};

    std::vector<ReflectedFile> reflectedFiles;
    
    filesystem::directory_iterator dirIter {m_reflectionDbFilePath};
    
    using DirIter = filesystem::directory_iterator;
    
    for (DirIter dirIter {m_reflectionDbFilePath}; dirIter != DirIter{}; ++dirIter) {
        std::cout << "JSON: " << dirIter->path() << "   "<< dirIter->path().extension() <<  std::endl;
        if (dirIter->path().extension().string() == jsonExtension) {
            reflectedFiles.emplace_back(readReflectedFile(dirIter->path()));
            std::cout << "added" << std::endl;
        }
    }
    
    return reflectedFiles;
}

void PersistentReflectionDB::addReflectedFile(const ReflectedFile& reflectedFile)
{
    using namespace boost;

    property_tree::ptree reflectedFileData;
    reflectedFileData.add(cppFilePathKey, reflectedFile.cppFilePath.string());
    reflectedFileData.add(reflectedCppFilePathKey, reflectedFile.reflectedCppFilePath.string());
    reflectedFileData.add(outFilePathKey, reflectedFile.outFilePath.string());
    reflectedFileData.add(functionNameKey, reflectedFile.functionName);
    
    create_directories(m_reflectionDbFilePath);
    auto path = m_reflectionDbFilePath;
    path.append(reflectedFile.functionName);
    path.replace_extension(jsonExtension);
    write_json(path.string(), reflectedFileData);
}
