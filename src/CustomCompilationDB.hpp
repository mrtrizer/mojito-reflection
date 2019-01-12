#pragma once

#include <map>
#include <boost/filesystem.hpp>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/ADT/StringRef.h>

class CustomCompilationDatabase : public clang::tooling::CompilationDatabase {
public:
    CustomCompilationDatabase(std::string compillerCommand, const boost::filesystem::path& cppPath, const std::multimap<std::string, std::string>& args);

    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef filePath) const override;
    
    std::vector<std::string> getAllFiles() const override;

    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override;
    
private:
    boost::filesystem::path m_cppPath;
    std::vector<std::string> m_args;
};
