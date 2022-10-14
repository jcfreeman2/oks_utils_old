#include <oks/attribute.h>
#include <oks/query.h>

int main()
{
  OksAttribute a(
        "Heigth",
        OksAttribute::float_type,
        false,
	"",
        "1.77",
        "person's heigth",
        true
  );
 
  OksOrExpression or_q;

	/* Looking for tall (h >= 1.88) and short (h <= 1.65) */

  or_q.add(new OksComparator(&a, new OksData((float)1.65), OksQuery::greater_or_equal_cmp));
  or_q.add(new OksComparator(&a, new OksData((float)1.88), OksQuery::less_or_equal_cmp));

  std::cout << or_q << std::endl;
 
  return 0;
}
