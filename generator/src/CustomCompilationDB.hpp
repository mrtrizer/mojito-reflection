#pragma once

#include <map>
#include <boost/filesystem.hpp>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/ADT/StringRef.h>

class CompillerArgs;
namespace boost::filesystem {
    class path;
}

class CustomCompilationDatabase : public clang::tooling::CompilationDatabase {
public:
    CustomCompilationDatabase(const boost::filesystem::path& compillerPath, const CompillerArgs& compillerArgs);

    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef filePath) const override;
    
    std::vector<std::string> getAllFiles() const override;

    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override;
    
private:
    const CompillerArgs& m_compillerArgs;
    boost::filesystem::path m_compillerPath;
};