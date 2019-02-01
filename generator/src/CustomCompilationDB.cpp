#include "CustomCompilationDB.hpp"

#include <iostream>
#include <regex>

#include "CompilerArgs.hpp"
#include "CompilerInfo.hpp"

#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>

CustomCompilationDatabase::CustomCompilationDatabase(const CompilerInfo& info, const CompilerArgs& compillerArgs)
    : m_compillerArgs(compillerArgs)
    , m_compilerInfo(info)
{}

std::vector<clang::tooling::CompileCommand> CustomCompilationDatabase::getCompileCommands(llvm::StringRef filePath) const {
    auto tmpCompillerArgs = m_compillerArgs;
    tmpCompillerArgs.setCppInputFiles({ boost::filesystem::path(filePath) });

    for (const auto& include : m_compilerInfo.includeDirs())
        tmpCompillerArgs.addIncludePath(include.string());
    
    std::vector<std::string> fullCommand {"c++"};
    auto arguments = tmpCompillerArgs.clangArguments();
    fullCommand.insert(fullCommand.end(), arguments.begin(), arguments.end());
    std::cout << "Full command: ";
    for (const auto& part : fullCommand)
        std::cout << part << ' ';
    std::cout << std::endl;
    
    std::vector<clang::tooling::CompileCommand> commands {
        clang::tooling::CompileCommand {
            boost::filesystem::current_path().string(),
            filePath,
            fullCommand,
            ""}
    };

    return commands;
}

std::vector<std::string> CustomCompilationDatabase::getAllFiles() const {
    std::vector<std::string> compillerArgs;
    std::transform(
        m_compillerArgs.cppInputFiles().begin(),
        m_compillerArgs.cppInputFiles().end(),
        std::back_inserter(compillerArgs),
        []( const auto& input){ return input.string(); });
    return compillerArgs;
}

std::vector<clang::tooling::CompileCommand> CustomCompilationDatabase::getAllCompileCommands() const {
    std::vector<clang::tooling::CompileCommand> commands;
    for (const auto& cpp : m_compillerArgs.cppInputFiles()) {
        auto commandsForCpp = getCompileCommands(cpp.string());
        commands.insert(commands.end(), commandsForCpp.begin(), commandsForCpp.end());
    }
    return commands;
}

