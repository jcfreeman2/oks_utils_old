#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/query.h>

int main()
{
  OksRelationship r(
	"has car", "Car",
	OksRelationship::Zero, OksRelationship::Many,
	true, true, false,
	"A person has zero or more cars"
  );

  OksAttribute a(
	"Type",
	OksAttribute::string_type,
	false,
	"",
	"unknown",
	"describes car type",
	true
  );

 
  OksRelationshipExpression rqe(&r, new OksComparator(&a, new OksData("BMW"), OksQuery::equal_cmp), false);

  std::cout << rqe << std::endl;
 
  return 0;
}
