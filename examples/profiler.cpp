#include <oks/kernel.h>
#include <oks/class.h>

int main(int argc, char **argv)
{
  OksKernel k;

  if(argc != 3) return 1;

  k.set_profiling_mode(true);
  k.load_schema(argv[1]);
  k.load_data(argv[2]);

  return 0;
}
