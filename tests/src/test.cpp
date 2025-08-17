#include "parser_test.hpp"
#include <boost/ut.hpp>
#include <pqrs/environment_variable.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "find"_test = [] {
    expect(pqrs::environment_variable::find("PATH") != std::nullopt);
    expect(pqrs::environment_variable::find("UNKNOWN_ENVIRONMENT_VARIABLE") == std::nullopt);
  };

  "load_environment_variables_from_file"_test = [] {
    setenv("HOME", "/Users/hello", 1);
    setenv("PATH", "/usr/sbin:/usr/bin:/sbin:/bin", 1);
    pqrs::environment_variable::load_environment_variables_from_file("data/environment", [](auto&& name, auto&& value) {
      std::cout << "setenv: " << name << " = " << value << std::endl;
    });

    expect(pqrs::environment_variable::find("PATH") == "/opt/homebrew/bin:/Users/hello/opt/bin:/usr/sbin:/usr/bin:/sbin:/bin"sv);
    expect(pqrs::environment_variable::find("XDG_CONFIG_HOME") == "/Users/hello/Library/Application Support/org.pqrs/config"sv);
    expect(pqrs::environment_variable::find("XDG_DATA_HOME") == "/Users/hello/Library/Application Support/org.pqrs/data"sv);
  };

  run_parser_test();

  return 0;
}
