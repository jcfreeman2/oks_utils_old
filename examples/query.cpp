#include <oks/kernel.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/query.h>

int main()
{
  try
    {
      OksKernel k; /* create OKS kernel */

      k.set_silence_mode(true);
      k.new_schema("/tmp/car.schema"); /* create new schema */
      k.new_data("/tmp/car.data"); /* create new data */

      /* define class 'Car' */
      OksClass * p = new OksClass("Car", "Describes a car", false, &k);

      /* define attribute 'Max Speed' */
      OksAttribute * a = new OksAttribute("Max Speed", OksAttribute::u16_int_type, false, "", "160", "max speed of car (km/h)", true);

      p->add(a);

      /* Creates three instances of Car class */
      OksObject * bmw316i = new OksObject(p, "BMW 316i");
      OksObject * bmw318i = new OksObject(p, "BMW 318i");
      OksObject * bmw320i = new OksObject(p, "BMW 320i");

      /* set max speeds */
      OksData d((uint16_t) 196);
      bmw316i->SetAttributeValue("Max Speed", &d);
      d.Set((uint16_t) 201);
      bmw318i->SetAttributeValue("Max Speed", &d);
      d.Set((uint16_t) 214);
      bmw320i->SetAttributeValue("Max Speed", &d);

      OksQuery q(false, new OksComparator(a, new OksData((unsigned short) 200), OksQuery::greater_cmp));

      std::list<OksObject *> * result = p->execute_query(&q);

      std::cout << "Query \'" << q << '\'' << ' ' << "found " << result->size() << ' ' << "objects in class \"" << p->get_name() << "\":\n";

      int i = 1;
      while (!result->empty())
        {
          OksObject * o = result->front();
          result->pop_front();
          std::cout << i++ << '.' << ' ' << *o;
        }

      delete result;

      OksObject::destroy(bmw320i);
      OksObject::destroy(bmw318i);
      OksObject::destroy(bmw316i);
    }
  catch (const std::exception & ex)
    {
      std::cerr << "Caught exception: " << ex.what() << std::endl;
    }

  return 0;
}
