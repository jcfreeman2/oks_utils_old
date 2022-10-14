#include <oks/attribute.h>
#include <oks/query.h>

int main()
{
  OksAttribute a(
        "Weight",
        OksAttribute::float_type,
        false,
	"",
        "75",
        "person's weight",
        true
  );
 
  OksAndExpression and_q;

	/* Looking for 60 >= weight <= 90 */

  and_q.add(new OksComparator(&a, new OksData((float)60.0), OksQuery::greater_or_equal_cmp));
  and_q.add(new OksComparator(&a, new OksData((float)90.0), OksQuery::less_or_equal_cmp));

  std::cout << and_q << std::endl;
 
  return 0;
}
