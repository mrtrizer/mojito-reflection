#include "GeneratorArgs.hpp"

#include <boost/program_options.hpp>

GeneratorArgs::GeneratorArgs(const std::vector<std::string>& args) {
    using namespace boost::program_options;

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

    basic_command_line_parser parser{args};
    parser.options(desc).allow_unregistered().style(
      command_line_style::default_style |
      command_line_style::allow_slash_for_short);
    parsed_options parsed_options = parser.run();

    variables_map vm;
    store(parsed_options, vm);

    m_unrecognized = collect_unrecognized(parsed_options.options, include_positional);
    
    if (vm.count(compiller) == 1)
        m_compillerPath = vm[compiller].as<std::string>();
    else
        throw std::runtime_error("Compiller is not defined");
    
    if (vm.count(reflectionIncludes) == 1)
        m_reflectionIncludesPath = vm[reflectionIncludes].as<std::string>();
    else
        throw std::runtime_error("Reflection includes path is not defined");

    if (vm.count(reflectionOut) == 1)
        m_reflectionOutPath = boost::filesystem::absolute(vm[reflectionOut].as<std::string>()).normalize();
    else
        throw std::runtime_error("Reflection output is not defined");
    
}