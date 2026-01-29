#include <iostream>
#include <cmath>

int fibonacci(int n)
{
  if (n <= 1)
    return n;
  return fibonacci(n - 1) + fibonacci(n - 2);
}


int main()
{
  int a = 5;
  double t = exp(a);
  std::cout << "hello " << fibonacci(a) << std::endl;
  std::cout << t << std::endl;
}
