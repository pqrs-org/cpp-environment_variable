#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <pqrs/environment_variable.hpp>

TEST_CASE("find") {
  REQUIRE(pqrs::environment_variable::find("PATH"));
  REQUIRE(pqrs::environment_variable::find("UNKNOWN_ENVIRONMENT_VARIABLE") == std::nullopt);
}
