#include "parser_test.hpp"
#include <boost/ut.hpp>
#include <pqrs/environment_variable.hpp>
#include <string>
#include <utility>
#include <vector>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "find"_test = [] {
    scoped_environment_variable variable("PQRS_ENVIRONMENT_VARIABLE_TEST_FIND");
    scoped_environment_variable unknown("PQRS_ENVIRONMENT_VARIABLE_TEST_UNKNOWN");

    expect(setenv("PQRS_ENVIRONMENT_VARIABLE_TEST_FIND", "value", 1) == 0_i);
    expect(unsetenv("PQRS_ENVIRONMENT_VARIABLE_TEST_UNKNOWN") == 0_i);

    expect(pqrs::environment_variable::find("PQRS_ENVIRONMENT_VARIABLE_TEST_FIND") == "value"sv);
    expect(pqrs::environment_variable::find("PQRS_ENVIRONMENT_VARIABLE_TEST_UNKNOWN") == std::nullopt);
  };

  "load_environment_variables_from_file"_test = [] {
    scoped_environment_variable home("PQRS_ENVIRONMENT_VARIABLE_TEST_HOME");
    scoped_environment_variable path("PQRS_ENVIRONMENT_VARIABLE_TEST_PATH");
    scoped_environment_variable config_home("PQRS_ENVIRONMENT_VARIABLE_TEST_CONFIG_HOME");
    scoped_environment_variable data_home("PQRS_ENVIRONMENT_VARIABLE_TEST_DATA_HOME");

    expect(setenv("PQRS_ENVIRONMENT_VARIABLE_TEST_HOME", "/Users/hello", 1) == 0_i);
    expect(setenv("PQRS_ENVIRONMENT_VARIABLE_TEST_PATH", "/usr/sbin:/usr/bin:/sbin:/bin", 1) == 0_i);
    expect(unsetenv("PQRS_ENVIRONMENT_VARIABLE_TEST_CONFIG_HOME") == 0_i);
    expect(unsetenv("PQRS_ENVIRONMENT_VARIABLE_TEST_DATA_HOME") == 0_i);

    std::vector<std::pair<std::string, std::string>> callbacks;
    pqrs::environment_variable::load_environment_variables_from_file("data/environment", [&](auto&& name, auto&& value) {
      std::cout << "setenv: " << name << " = " << value << std::endl;
      callbacks.emplace_back(name, value);
    });

    std::vector<std::pair<std::string, std::string>> expected_callbacks{
        {"PQRS_ENVIRONMENT_VARIABLE_TEST_PATH", "/Users/hello/opt/bin:/usr/sbin:/usr/bin:/sbin:/bin"},
        {"PQRS_ENVIRONMENT_VARIABLE_TEST_PATH", "/opt/homebrew/bin:/Users/hello/opt/bin:/usr/sbin:/usr/bin:/sbin:/bin"},
        {"PQRS_ENVIRONMENT_VARIABLE_TEST_CONFIG_HOME", "/Users/hello/Library/Application Support/org.pqrs/config"},
        {"PQRS_ENVIRONMENT_VARIABLE_TEST_DATA_HOME", "/Users/hello/Library/Application Support/org.pqrs/data"},
    };
    bool callbacks_match_expected = callbacks == expected_callbacks;
    expect(callbacks_match_expected);

    expect(pqrs::environment_variable::find("PQRS_ENVIRONMENT_VARIABLE_TEST_PATH") == "/opt/homebrew/bin:/Users/hello/opt/bin:/usr/sbin:/usr/bin:/sbin:/bin"sv);
    expect(pqrs::environment_variable::find("PQRS_ENVIRONMENT_VARIABLE_TEST_CONFIG_HOME") == "/Users/hello/Library/Application Support/org.pqrs/config"sv);
    expect(pqrs::environment_variable::find("PQRS_ENVIRONMENT_VARIABLE_TEST_DATA_HOME") == "/Users/hello/Library/Application Support/org.pqrs/data"sv);
  };

  "load_environment_variables_from_missing_file"_test = [] {
    std::vector<std::pair<std::string, std::string>> callbacks;
    pqrs::environment_variable::load_environment_variables_from_file("data/missing_environment", [&](auto&& name, auto&& value) {
      callbacks.emplace_back(name, value);
    });

    expect(callbacks.empty());
  };

  run_parser_test();

  return 0;
}
