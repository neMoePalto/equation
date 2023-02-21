#include <iostream>
#include <string>

#include "equation.h"


int main(int argc, char* argv[]) {
  std::string str;

  if (argc < 2) {
    std::cout << "Error: to few arguments, 1 requred" << std::endl;
    return -1;
  } else if (argc > 2) {
    std::cout << "Error: to many arguments, 1 requred" << std::endl;
    return -1;
  } else {
    str = argv[1];
  }

  try {
    equation eq(str);
    double x = eq.get_x();
    std::cout << x << std::endl;
  } catch (const std::runtime_error& e) {
    std::cout << e.what() << std::endl;
    return -1;
  }

  return 0;
}
