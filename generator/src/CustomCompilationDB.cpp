#include "CustomCompilationDB.hpp"

#include <iostream>
#include <regex>

#include "CompillerArgs.hpp"

#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>

static std::vector<boost::filesystem::path> getClangIncludeDirs(const boost::filesystem::path& compillerPath) {
    using namespace boost;

    std::vector<boost::filesystem::path> includeDirs;

    try {
        process::ipstream pipe_stream;
        
        std::string command = compillerPath.string() + " -v";
        
        process::child c(command, process::std_err > pipe_stream);

        std::string clangInstallDir;
        std::string version;

        std::string line;
        while (!pipe_stream.eof() && std::getline(pipe_stream, line) && !line.empty()) {
            trim(line);
            
            std::cmatch m;
            
            std::regex re("version ([0-9]*?\\.[0-9]*?\\.[0-9]*)");
            
            if (std::regex_search(line.c_str(), m, re))
                version = m[1];
            
            const char installedDirMarker[] = "InstalledDir: ";
            if (line.find(installedDirMarker) == 0)
                clangInstallDir = line.substr(sizeof(installedDirMarker) / sizeof(installedDirMarker[0]) - 1);
        }
        
        if (version.empty())
            throw std::runtime_error("Can't get clang version by path " + compillerPath.string());
        
        if (clangInstallDir.empty())
            throw std::runtime_error("Can't access clang by path " + compillerPath.string());

        includeDirs.emplace_back(clangInstallDir + "/../include/c++/v1").normalize();
        includeDirs.emplace_back(clangInstallDir + "/../lib/clang/" + version + "/include").normalize();
    } catch (const std::exception& e) {
        std::cout << "Can't read installation dir and includes for compiller: " << compillerPath << std::endl;
        throw;
    }

    return includeDirs;
}

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
    auto arguments = m_compillerArgs.clangArguments();
    arguments.insert(arguments.begin(), "c++");
    auto includes = getClangIncludeDirs(m_compillerPath);
    for (const auto& include : includes) {
        arguments.push_back("-I");
        arguments.push_back(include.string());
    }
    //arguments.push_back("-I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1");
    for (const auto& arg : arguments)
        std::cout << arg << ' ';
    std::cout << std::endl;
    for (const auto& cpp : m_compillerArgs.cppInputFiles())
        commands.emplace_back(clang::tooling::CompileCommand {
                                boost::filesystem::current_path().string(),
                                cpp.string(),
                                arguments,
                                ""});
    return commands;
}

