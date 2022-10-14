#include <oks/attribute.h>
#include <oks/query.h>

int main()
{
  try
    {
      const OksAttribute a("Name", OksAttribute::string_type, false, "", "unknown", "describes address", true);

      OksComparator qc(&a, new OksData("Peter"), OksQuery::equal_cmp);

      std::cout << qc << std::endl;
    }
  catch (const std::exception& ex)
    {
      std::cerr << "Caught exception:\n" << ex.what() << std::endl;
    }
 
  return 0;
}
