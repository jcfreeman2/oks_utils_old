#include <oks/kernel.h>
#include <oks/class.h>

int main(int argc, char **argv)
{
  OksKernel k;

  if(argc != 2) return 1;

  k.load_schema(argv[1]);

  std::cout << "Schema file contains:\n";
  for(OksClass::Map::const_iterator i = k.classes().begin(); i != k.classes().end(); ++i)
    std::cout << "\t\"" << i->first << "\" class\n";

  return 0;
}
