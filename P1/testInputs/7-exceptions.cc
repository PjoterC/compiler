#include <iostream>

struct exception {};

struct div_by_0_exception : exception {
  double dividend;
};

double div(double dividend, double divisor);

int main() {
  double dividend, divisor;

  std::cout << "dividend: ";
  std::cin >> dividend;

  std::cout << "divisor: ";
  std::cin >> divisor;

  try {
    const double result = div(dividend, divisor);
    std::cout << result << std::endl;
  } catch (const div_by_0_exception& e) {
    std::cerr << "Error! Tried to divide " << e.dividend << " by 0!" << std::endl;
    return -1;
  }

  return 0;
}
