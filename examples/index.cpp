#include <oks/kernel.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/query.h>

#include <stdlib.h>

#include <sstream>
 

int main()
{
  try
    {
      // create OKS kernel
      OksKernel k;

      // create new schema and data files
      k.new_schema("/tmp/index.schema");
      k.new_data("/tmp/index.data");

      // define class 'Randomizer'
      OksClass * p = new OksClass("Randomizer", "Describes a Randomizer", false, &k);

      // define attribute 'Value'
      OksAttribute * a = new OksAttribute("Value", OksAttribute::double_type, false, "", "0.5", "random value", false);

      p->add(a);


      // Create 100,000 instances of the class
      size_t i = 0;
      OksDataInfo * odi = p->data_info("Value");

      while (i++ < 100000)
        {
          std::ostringstream s;
          s << i;

          std::string buf = s.str();

          OksObject *o = new OksObject(p, buf.c_str());
          OksData d((double) (random() % (1 << 16)) / (double) (1 << 16));

          o->SetAttributeValue(odi, &d);
        }

      // Create index for attribute 'Value'
      OksIndex index(p, a);

      // Search values >= 0.9 using index
      OksData d(double(0.9));
      std::list<OksObject *> * result = index.FindGreatEqual(&d);

      // Prints number of found instances
      // The expected value should be about 10,000
      if (result)
        {
          std::cout << "Found " << result->size() << " instances >= 0.9\n";
          delete result;
        }
    }
  catch (const std::exception & ex)
    {
      std::cerr << "Caught exception: " << ex.what() << std::endl;
    }

  return 0;
}
