#include <boost/ut.hpp>
#include <pqrs/environment_variable.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "find"_test = [] {
    expect(pqrs::environment_variable::find("PATH") != std::nullopt);
    expect(pqrs::environment_variable::find("UNKNOWN_ENVIRONMENT_VARIABLE") == std::nullopt);
  };

  return 0;
}
