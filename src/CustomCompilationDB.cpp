#include "CustomCompilationDB.hpp"

#include "CompillerArgs.hpp"

CustomCompilationDatabase::CustomCompilationDatabase(const boost::filesystem::path& compillerPath, const CompillerArgs& compillerArgs)
    : m_compillerArgs(compillerArgs)
    , m_compillerPath(compillerPath)
{}

std::vector<clang::tooling::CompileCommand> CustomCompilationDatabase::getCompileCommands(llvm::StringRef filePath) const {
    return getAllCompileCommands();
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
    for (const auto& cpp : m_compillerArgs.cppInputFiles())
        commands.emplace_back(clang::tooling::CompileCommand {
                                boost::filesystem::current_path().string(),
                                cpp.string(),
                                m_compillerArgs.allArguments(),
                                ""});
    return commands;
}

