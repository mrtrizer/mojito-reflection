#pragma once

#include <map>
#include <boost/filesystem.hpp>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/ADT/StringRef.h>

#include "CompilerArgs.hpp"
#include "CompilerInfo.hpp"

class CustomCompilationDatabase : public clang::tooling::CompilationDatabase {
public:
    CustomCompilationDatabase(const CompilerInfo& compilerInfo, const CompilerArgs& compillerArgs);

    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef filePath) const override;
    
    std::vector<std::string> getAllFiles() const override;

    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override;
    
private:
    CompilerArgs m_compillerArgs;
    CompilerInfo m_compilerInfo;
};
