#include <oks/class.h>
#include <oks/attribute.h>
#include <oks/relationship.h>

int main()
{
  try
    {
      OksClass * c = new OksClass(
            "Person", /* class name */
            "Describes a person", /* description */
            false, /* is not abstract */
            0 /* no kernel */
      );
	
      OksAttribute * a = new OksAttribute(
            "Name",
            OksAttribute::string_type,
            false,
            "",
            "Unknown",
            "Describes person name",
            true
      );

      OksRelationship * r = new OksRelationship(
            "Works at",
            "Department",
            OksRelationship::Zero, OksRelationship::Many,
            false, false, false,
            "Can have many work places"
      );

      c->add(a); /* add attribute to class */
      c->add(r); /* add relationship to class */

      std::cout << "Class description is:\n" << *c << std::endl;

      OksClass::destroy(c);
    }
  catch (const std::exception & ex)
    {
      std::cerr << "Caught exception: " << ex.what() << std::endl;
    }

  return 0;
}
