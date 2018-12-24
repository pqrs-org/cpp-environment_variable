#include <iostream>
#include <pqrs/environment_variable.hpp>

int main(void) {
  if (auto home = pqrs::environment_variable::find("HOME")) {
    std::cout << "HOME: " << *home << std::endl;
  }

  return 0;
}
