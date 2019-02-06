#include "GeneratorArgs.hpp"

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/process.hpp>

GeneratorArgs::GeneratorArgs(const std::vector<std::string>& args) {
    using namespace boost::program_options;
    using namespace boost;

    const char* compiller = "compiller";
    const char* reflectionIncludes = "reflection-includes";
    const char* reflectionName = "reflection-name";
    const char* reflectionOut = "reflection-out";

    options_description desc{"Options"};
    desc.add_options()
      (compiller, value<std::string>()->required())
      (reflectionIncludes, value<std::string>()->required())
      (reflectionName, value<std::string>())
      (reflectionOut, value<std::string>());

    variables_map vm;
    basic_command_line_parser parser{args};
    parser.options(desc).allow_unregistered().style(
      command_line_style::default_style |
      command_line_style::allow_slash_for_short);
    try {
        parsed_options parsed_options = parser.run();
        store(parsed_options, vm);
        m_unrecognized = collect_unrecognized(parsed_options.options, include_positional);
    } catch (const std::exception& e) {
        std::cout << "Failed to parse arguments" << std::endl;
        throw;
    }

    if (vm.count(compiller) == 1) {
        filesystem::path compillerValue = vm[compiller].as<std::string>();
        if (compillerValue.has_parent_path())
            m_compillerPath = filesystem::absolute(compillerValue).normalize();
        else
            m_compillerPath = process::search_path(compillerValue);
        std::cout << "Detected compiller: " << m_compillerPath << std::endl;
    } else {
        throw std::runtime_error("Compiller is not defined");
    }
    
    if (vm.count(reflectionIncludes) == 1)
        m_reflectionIncludesPath = vm[reflectionIncludes].as<std::string>();
    else
        throw std::runtime_error("Reflection includes path is not defined");

    if (vm.count(reflectionOut) == 1)
        m_reflectionOutPath = boost::filesystem::absolute(vm[reflectionOut].as<std::string>()).normalize();
    else
        throw std::runtime_error("Reflection output is not defined");
    
    if (vm.count(reflectionName) == 1)
        m_reflectionName = vm[reflectionName].as<std::string>();
    else
        throw std::runtime_error("Reflection name is not defined");
    
}
