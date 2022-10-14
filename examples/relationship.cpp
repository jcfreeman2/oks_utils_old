#include <oks/relationship.h>

int main()
{
  try
    {
      OksRelationship r(
	"consists of", /* name */
	"Element", /* class type */
	OksRelationship::Zero, /* low cc in Zero */
	OksRelationship::Many, /* high cc is Many */
	true, /* is composite */
	true, /* is exclusive */
	true, /* is dependent */
	"A structure consists of zero or many elements" /* description */
      );

      std::cout << r;
    }
  catch (const std::exception& ex)
    {
      std::cerr << "Caught exception:\n" << ex.what() << std::endl;
    }

  return 0;
}
