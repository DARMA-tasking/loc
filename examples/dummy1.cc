#include <iostream>
#include <loc/dummy.h>

int main() {
  loc::dummy::Dummy dummy;

  std::cout << dummy.sum(2, 8) << '\n';

  return 0;
}
