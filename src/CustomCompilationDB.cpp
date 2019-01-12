#include "CustomCompilationDB.hpp"

CustomCompilationDatabase::CustomCompilationDatabase(std::string compillerCommand, const boost::filesystem::path& cppPath, const std::multimap<std::string, std::string>& args)
    : m_cppPath(cppPath)
{
    m_args.emplace_back(compillerCommand);
    for (auto arg : args) {
        if (arg.first.empty())
            m_args.emplace_back(cppPath.string());
        else {
            m_args.emplace_back(arg.first);
            if (!arg.second.empty())
                m_args.emplace_back(arg.second);
        }
    }
}

std::vector<clang::tooling::CompileCommand> CustomCompilationDatabase::getCompileCommands(llvm::StringRef filePath) const {
    return getAllCompileCommands();
}

std::vector<std::string> CustomCompilationDatabase::getAllFiles() const {
    return { m_cppPath.filename().string() };
}

std::vector<clang::tooling::CompileCommand> CustomCompilationDatabase::getAllCompileCommands() const {
    return { clang::tooling::CompileCommand { boost::filesystem::current_path().string(), m_cppPath.filename().string(), m_args, "" } };
}

