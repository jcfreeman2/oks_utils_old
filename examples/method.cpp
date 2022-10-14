#include <oks/method.h>

int main()
{
  OksMethod m(
    "print",       /* name */
    "it is a test" /* description */
  );

  m.add_implementation(
    "c++",
    "void print(const char * s)",
    "std::cout << s << std::endl;"
  );

  m.add_implementation(
    "c",
    "void print(const char * s)",
    "printf(\"%s\\n\", s);"
  );

  std::cout << m;

  return 0;
}
