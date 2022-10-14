#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <chrono>
#include <fstream>

#include <oks/kernel.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/query.h>


struct CallTimeInfo {
  CallTimeInfo(const char *s, double d) : str (s), t (d) {;}

  bool operator==(const CallTimeInfo &c) const {
    return (
      (
        (this == &c) ||
        (t == c.t && str == c.str)
      ) ? true : false
    );
  };
  
  friend std::ostream& operator<<(std::ostream&, const CallTimeInfo &);

  const char *str;
  double t;
};

std::ostream& operator<<(std::ostream& s, const CallTimeInfo& c)
{
  double t = c.t;

  s.precision(4);
  s.fill(' ');
  s.width(40);
  s.setf(std::ios::left, std::ios::adjustfield);
  s << c.str;

  s << t << " ms\n";

  return s;
}

std::ostream&
test_start_msg(int& count) {
  std::cout << "[TEST #" << ++count<< "]: ";
  
  return std::cout;
}

void
test_start_msg(int& count, const char *msg) {
  test_start_msg(count) << msg << "...";
}

OksClass*
find_class_and_report(OksKernel& kernel, const std::string& name)
{
  OksClass *c = kernel.find_class(name);
  
  if(!c) {
    Oks::error_msg("3") << "Can`t find class \"" << name << "\", exiting ...\n";
    exit(3);
  }
  
  return c;
}



#define TIME_TEST_START(N)				\
{							\
  unsigned long j;					\
  auto tp = std::chrono::steady_clock::now();		\
  for(j=0;j<N;j++) {

#define TIME_TEST_STOP(STR)                                                                                                          \
  }                                                                                                                                  \
  double time_per_call = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-tp).count() / 1000.; \
  std::cout << " (" << time_per_call << " ms)\n";                                                                                    \
  std::cout.flush();                                                                                                                 \
  time_per_call /= (double)j;                                                                                                        \
  table.push_back(new CallTimeInfo(STR, time_per_call));                                                                             \
}


int main(int argc, char **argv)
{
    // parse command line and get paths to schema and data files

  const char *schemaFile = 0;
  const char *dataFile = 0;

  {
    for(int j = 1; j < argc; j++) {
      if(!strcmp(argv[j], "-schema"))
        schemaFile = argv[++j];
      else if(!strcmp(argv[j], "-data"))
        dataFile = argv[++j];
      else
        std::cerr << "Unknown parameter: \"" << argv[j] << "\"\n";
    }

    if(!schemaFile) schemaFile = PATH_TO_SCHEMA;
    if(!dataFile) dataFile = "test.data";
  }

  int countOfTests = 0;
  std::list<CallTimeInfo *> table;

  { /* Construct kernel */

  OksKernel kernel;

	//
	// Before start tests we have to execute performance
	// benchmark to calculate speed factor
	//
	// 'performance' shows how many times body of loop
	// below will be executed during one second
	//

  size_t performance = 1;

  {
	const size_t numberOfTests = 10000;

        auto tp = std::chrono::steady_clock::now();

	for(size_t j=0; j<numberOfTests; j++) {
		OksData *char_data		= new OksData('A');
		OksData *int_data		= new OksData((short)12345);
		OksData *float_data		= new OksData((float)9876543);
		OksData *string_data		= new OksData("Hello OKS");
		OksData::List * dlist		= new OksData::List();
		dlist->push_back(char_data);
		dlist->push_back(int_data);
		dlist->push_back(float_data);
		dlist->push_back(string_data);

		OksData list_data(dlist);
	}


	performance = (size_t)((double)numberOfTests / std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-tp).count());
	if(performance < 10000) performance = 10000;
	
	std::cout << "Performance factor is " << performance << std::endl;
  }

  /**************************************************************************/

  OksFile * schema_h = 0;

  TIME_TEST_START(1)

    try {
      schema_h = kernel.load_schema(schemaFile);
    }
    catch (oks::exception & ex) {
      Oks::error_msg("1") << ex.what() << std::endl;
      return 1;
    }

    test_start_msg(countOfTests, "Load schema file");

  TIME_TEST_STOP("load_schema")

  /**************************************************************************/

  OksFile * data_h = 0;

  TIME_TEST_START(1)

    try {
      data_h = kernel.load_data(dataFile);
    }
    catch (oks::exception & ex) {
      Oks::error_msg("1") << ex.what() << std::endl;
      return 2;
    }

    test_start_msg(countOfTests, "Load data file");

  TIME_TEST_STOP("load_data")

  /**************************************************************************/

  TIME_TEST_START(1)

    kernel.close_all_data();

    test_start_msg(countOfTests, "Close data file");

  TIME_TEST_STOP("close_data")

  /**************************************************************************/

  std::string new_data_file = std::string(dataFile) + ".ext";

  TIME_TEST_START(1)

    try {
      data_h = kernel.load_data(new_data_file);
    }
    catch (oks::exception & ex) {
      Oks::error_msg("1") << ex.what() << std::endl;
      return 2;
    }

    test_start_msg(countOfTests, "Load data file");

  TIME_TEST_STOP("load_data_ext")

  /**************************************************************************/



  test_start_msg(countOfTests, "Find class by name");

  OksClass *SWObjectClass = 0;
  OksClass *ProgramClass = 0;
  OksClass *EnvironmentClass = 0;

  {
    size_t numberOfRepet = performance;

    const std::string sw_obj_class_name("SW_Object");
    const std::string program_class_name("Program");
    const std::string sw_res_class_name("SW_Resource");
    const std::string process_class_name("Process");
    const std::string computer_class_name("Computer");
    const std::string appl_class_name("Application");
    const std::string env_class_name("Environment");

    TIME_TEST_START(numberOfRepet)

      SWObjectClass = find_class_and_report(kernel, sw_obj_class_name);
      ProgramClass = find_class_and_report(kernel, program_class_name);
      EnvironmentClass = find_class_and_report(kernel, env_class_name);
      find_class_and_report(kernel, sw_res_class_name);
      find_class_and_report(kernel, process_class_name);
      find_class_and_report(kernel, computer_class_name);
      find_class_and_report(kernel, appl_class_name);

    TIME_TEST_STOP("find_class")
  }
 
  table.back()->t /= 7;

  /**************************************************************************/

  test_start_msg(countOfTests, "Access object by OID");

  unsigned long		numOfObjs = kernel.number_of_objects();
  unsigned long		OBJS_NUM = numOfObjs / 10;
  OksObject		**objs = new OksObject * [OBJS_NUM+1];
  unsigned long		numberOfRepet = (2 * performance) / OBJS_NUM;
  size_t		i = 0;
  
  const OksObject::Set & all_objs = kernel.objects();

  for(OksObject::Set::const_iterator oi = all_objs.begin(); oi != all_objs.end(); ++oi)
    if(!((++i)%10))
      objs[(i/10)-1] = *oi;

  if(!numberOfRepet) numberOfRepet = 1;

  /**************************************************************************/

  {
    TIME_TEST_START(numberOfRepet)

      for(i=0; i<OBJS_NUM; ++i)
        objs[i]->GetClass()->get_object(&objs[i]->GetId());

    TIME_TEST_STOP("get_object")
  }

  table.back()->t /= OBJS_NUM;

  delete [] objs;

  /**************************************************************************/

  try {
    kernel.set_active_data(data_h);
  }
  catch (oks::exception & ex) {
    Oks::error_msg("4") << ex.what() << std::endl;
    return 4;
  }

  /**************************************************************************/
 
  OksData	*d;
  unsigned long	sw_num = SWObjectClass->number_of_objects();
  OksObject	**SWObjectClassObjects = new OksObject * [sw_num];

  const OksObject::Map * sw_objs = SWObjectClass->objects();

  i = 0;
  for(OksObject::Map::const_iterator swoi = sw_objs->begin(); swoi != sw_objs->end(); ++swoi)
    SWObjectClassObjects[i++] = (*swoi).second;

  numberOfRepet = 10000;
  

  /**************************************************************************/

  std::string Name_s("Name");
  std::string Description_s("Description");
  std::string Authors_s("Authors");
  std::string HelpLink_s("HelpLink");
  std::string DefaultParameter_s("DefaultParameter");
  std::string DefaultPriority_s("DefaultPriority");
  std::string DefaultPrivileges_s("DefaultPrivileges");

  OksDataInfo *Name_odi			= SWObjectClass->data_info(Name_s);
  OksDataInfo *Description_odi		= SWObjectClass->data_info(Description_s);
  OksDataInfo *Authors_odi		= SWObjectClass->data_info(Authors_s);
  OksDataInfo *HelpLink_odi		= SWObjectClass->data_info(HelpLink_s);
  OksDataInfo *DefaultParameter_odi	= SWObjectClass->data_info(DefaultParameter_s);
  OksDataInfo *DefaultPriority_odi	= SWObjectClass->data_info(DefaultPriority_s);
  OksDataInfo *DefaultPrivileges_odi	= SWObjectClass->data_info(DefaultPrivileges_s);


  std::string ImplementedBy_s("ImplementedBy");
  std::string NeedsResources_s("NeedsResources");
  std::string NeedsEnvironment_s("NeedsEnvironment");

  OksDataInfo *ImplementedBy_odi	= SWObjectClass->data_info(ImplementedBy_s);
  OksDataInfo *NeedsResources_odi	= SWObjectClass->data_info(NeedsResources_s);
  OksDataInfo *NeedsEnvironment_odi	= SWObjectClass->data_info(NeedsEnvironment_s);

  /**************************************************************************/

  test_start_msg(countOfTests, "Get value of attribute by name");

  {
    size_t numberOfRepet = (1 + ((performance * 2) / sw_num));
    
    TIME_TEST_START(numberOfRepet)

      for(i=0; i<sw_num; ++i) {
        d = SWObjectClassObjects[i]->GetAttributeValue(Name_s);
        d = SWObjectClassObjects[i]->GetAttributeValue(Description_s);
        d = SWObjectClassObjects[i]->GetAttributeValue(Authors_s);
        d = SWObjectClassObjects[i]->GetAttributeValue(HelpLink_s);
        d = SWObjectClassObjects[i]->GetAttributeValue(DefaultParameter_s);
        d = SWObjectClassObjects[i]->GetAttributeValue(DefaultPriority_s);
        d = SWObjectClassObjects[i]->GetAttributeValue(DefaultPrivileges_s);
      }

    TIME_TEST_STOP("get_attribute_value_by_name")

    table.back()->t /= 7;
    table.back()->t /= sw_num;
  }

  /**************************************************************************/

  test_start_msg(countOfTests, "Get value of attribute by OID");

  {  
    size_t numberOfRepet = (1 + ((performance * 100) / sw_num));
    
    TIME_TEST_START(numberOfRepet)

      for(i=0; i<sw_num; ++i) {
        d = SWObjectClassObjects[i]->GetAttributeValue(Name_odi);
        d = SWObjectClassObjects[i]->GetAttributeValue(Description_odi);
        d = SWObjectClassObjects[i]->GetAttributeValue(Authors_odi);
        d = SWObjectClassObjects[i]->GetAttributeValue(HelpLink_odi);
        d = SWObjectClassObjects[i]->GetAttributeValue(DefaultParameter_odi);
        d = SWObjectClassObjects[i]->GetAttributeValue(DefaultPriority_odi);
        d = SWObjectClassObjects[i]->GetAttributeValue(DefaultPrivileges_odi);
      }

    TIME_TEST_STOP("get_attribute_value_by_oid")

    table.back()->t /= 7;
    table.back()->t /= sw_num;
  }
  
  double time_for_get_attribute_value_by_oid = table.back()->t;

  /**************************************************************************/

  test_start_msg(countOfTests, "Get value of relationship by name");

  {
    size_t numberOfRepet = (1 + ((performance * 2) / sw_num));
    
    TIME_TEST_START(numberOfRepet)

      for(i=0; i<sw_num; ++i) {
        d = SWObjectClassObjects[i]->GetRelationshipValue(ImplementedBy_s);
        d = SWObjectClassObjects[i]->GetRelationshipValue(NeedsResources_s);
        d = SWObjectClassObjects[i]->GetRelationshipValue(NeedsEnvironment_s);
      }

    TIME_TEST_STOP("get_relationship_value_by_name")

    table.back()->t /= 3;
    table.back()->t /= sw_num;
  }

  /**************************************************************************/

  test_start_msg(countOfTests, "Get value of relationship by OID");

  {
    size_t numberOfRepet = (1 + ((performance * 10) / sw_num));
    
    TIME_TEST_START(numberOfRepet)

      for(i=0; i<sw_num; ++i) {
        d = SWObjectClassObjects[i]->GetRelationshipValue(ImplementedBy_odi);
        d = SWObjectClassObjects[i]->GetRelationshipValue(NeedsResources_odi);
        d = SWObjectClassObjects[i]->GetRelationshipValue(NeedsEnvironment_odi);
      }

    TIME_TEST_STOP("get_relationship_value_by_oid")

    table.back()->t /= 3;
    table.back()->t /= sw_num;
  }

  /**************************************************************************/

  OksData d_Name("a server");
  OksData d_Desc("This is a test server");
  OksData d_Auth(new OksData::List()); d_Auth.data.LIST->push_back(new OksData("atdsoft"));
  OksData d_Help("http://atddoc.cern.ch/Atlas");
  OksData d_Prms("-test -n");
  OksData d_Prir((int32_t)0L);
  OksData d_Priv("");
  
  /**************************************************************************/

  test_start_msg(countOfTests, "Set value of attribute by name");

  {
    size_t numberOfRepet = (1 + ((performance) / sw_num));
    
    TIME_TEST_START(numberOfRepet)

      for(i=0; i<sw_num; ++i) {
        SWObjectClassObjects[i]->SetAttributeValue(Name_s, &d_Name);
        SWObjectClassObjects[i]->SetAttributeValue(Description_s, &d_Desc);
        SWObjectClassObjects[i]->SetAttributeValue(Authors_s, &d_Auth);
        SWObjectClassObjects[i]->SetAttributeValue(HelpLink_s, &d_Help);
        SWObjectClassObjects[i]->SetAttributeValue(DefaultParameter_s, &d_Prms);
        SWObjectClassObjects[i]->SetAttributeValue(DefaultPriority_s, &d_Prir);
        SWObjectClassObjects[i]->SetAttributeValue(DefaultPrivileges_s, &d_Priv);
      }

    TIME_TEST_STOP("set_attribute_value_by_name")
  }
  table.back()->t /= 7 * sw_num;

  /**************************************************************************/

  test_start_msg(countOfTests, "Set value of relationship by name");
  
  {
    size_t numberOfRepet = (1 + ((performance)  / sw_num));

    TIME_TEST_START(numberOfRepet)

      for(i=0; i<sw_num; ++i) {
        d = SWObjectClassObjects[i]->GetRelationshipValue(ImplementedBy_s);
        SWObjectClassObjects[i]->SetRelationshipValue(ImplementedBy_s, d);

        d = SWObjectClassObjects[i]->GetRelationshipValue(NeedsResources_s);
        SWObjectClassObjects[i]->SetRelationshipValue(NeedsResources_s, d);

        d = SWObjectClassObjects[i]->GetRelationshipValue(NeedsEnvironment_s);
        SWObjectClassObjects[i]->SetRelationshipValue(NeedsEnvironment_s, d);
      }

    TIME_TEST_STOP("set_relationship_value_by_name")
  }
  table.back()->t /= 3 * sw_num;
  table.back()->t -= time_for_get_attribute_value_by_oid;

  /**************************************************************************/

  test_start_msg(countOfTests, "Set value of attribute by OID");

  {
    size_t numberOfRepet = (1 + ((performance * 2) / sw_num));

    TIME_TEST_START(numberOfRepet)

      for(i=0; i<sw_num; ++i) {
        SWObjectClassObjects[i]->SetAttributeValue(Name_odi, &d_Name);
        SWObjectClassObjects[i]->SetAttributeValue(Description_odi, &d_Desc);
        SWObjectClassObjects[i]->SetAttributeValue(Authors_odi, &d_Auth);
        SWObjectClassObjects[i]->SetAttributeValue(HelpLink_odi, &d_Help);
        SWObjectClassObjects[i]->SetAttributeValue(DefaultParameter_odi, &d_Prms);
        SWObjectClassObjects[i]->SetAttributeValue(DefaultPriority_odi, &d_Prir);
        SWObjectClassObjects[i]->SetAttributeValue(DefaultPrivileges_odi, &d_Priv);
      }

    TIME_TEST_STOP("set_attribute_value_by_oid")

    table.back()->t /= 7 * sw_num;
  }

  /**************************************************************************/

  test_start_msg(countOfTests, "Set value of relationship by OID");

  {
    size_t numberOfRepet = (1 + ((performance * 2) / sw_num));

    TIME_TEST_START(numberOfRepet)

      for(i=0; i<sw_num; ++i) {
        d = SWObjectClassObjects[i]->GetRelationshipValue(ImplementedBy_odi);
        SWObjectClassObjects[i]->SetRelationshipValue(ImplementedBy_odi, d);

        d = SWObjectClassObjects[i]->GetRelationshipValue(NeedsResources_odi);
        SWObjectClassObjects[i]->SetRelationshipValue(NeedsResources_odi, d);

        d = SWObjectClassObjects[i]->GetRelationshipValue(NeedsEnvironment_odi);
        SWObjectClassObjects[i]->SetRelationshipValue(NeedsEnvironment_odi, d);
      }

    TIME_TEST_STOP("set_relationship_value_by_oid")

    table.back()->t /= 3 * sw_num;
    table.back()->t -= time_for_get_attribute_value_by_oid;
  }

  /**************************************************************************/

	//
	// FIXME:
	// WNT Problems
	//

#ifdef _MSC_VER
  d_Auth.Clear2();
#endif

  /**************************************************************************/
  
  test_start_msg(countOfTests, "Create instances");
  
  unsigned long numberOfSWObj = SWObjectClass->number_of_objects();
  unsigned long numberOfPrObj = ProgramClass->number_of_objects();

  OksObject *objects[500];

  size_t numberOfNewObjs = (
	(performance < 1000)
		? 100
		: (performance < 10000)
			? 300
			: 500
  );
  
  TIME_TEST_START(1)
  
    for(i=0; i<numberOfNewObjs; ++i) {
      char objName[32];
      sprintf(objName, "%ld_obj", (long)i);
      objects[i] = new OksObject(SWObjectClass, objName);
    }

  TIME_TEST_STOP("object_constructor")

  table.back()->t /= numberOfNewObjs;
 
  for(i=0; i<numberOfNewObjs; ++i) {
    OksData *d;

    d = SWObjectClassObjects[rand() % sw_num]->GetRelationshipValue(NeedsResources_odi);
    objects[i]->SetRelationshipValue(NeedsResources_odi, d);

    d = SWObjectClassObjects[rand() % sw_num]->GetRelationshipValue(NeedsEnvironment_odi);
    objects[i]->SetRelationshipValue(NeedsEnvironment_odi, d);
        
    for(int k=0; k<5; k++) {
      char buf[32];
      sprintf(buf, "p%ld-%d", (long)i, k);
      objects[i]->AddRelationshipValue(ImplementedBy_odi, new OksObject(ProgramClass, buf));
    }
  }

  unsigned long numberOfSWObj2 = SWObjectClass->number_of_objects();
  unsigned long numberOfPrObj2 = ProgramClass->number_of_objects();

  std::cout << " - create " << (numberOfSWObj2 - numberOfSWObj) << " instances of \'" << SWObjectClass->get_name() << "\' class\n"
               " - create " << (numberOfPrObj2 - numberOfPrObj) << " instances of \'" << ProgramClass->get_name() << "\' class\n";

  /**************************************************************************/

  delete [] SWObjectClassObjects;

  /**************************************************************************/

  test_start_msg(countOfTests, "Remove instances");
  
  TIME_TEST_START(1)
  
  for(i=0; i<numberOfNewObjs; ++i) {
    OksObject::destroy(objects[i]);
  }

  TIME_TEST_STOP("object_destructor")

  table.back()->t /= numberOfNewObjs;

  numberOfSWObj = SWObjectClass->number_of_objects();
  numberOfPrObj = ProgramClass->number_of_objects();

  std::cout << " - remove " << (numberOfSWObj2 - numberOfSWObj) << " instances of \'" << SWObjectClass->get_name() << "\' class\n"
               " - remove " << (numberOfPrObj2 - numberOfPrObj) << " instances of \'" << ProgramClass->get_name() << "\' class\n";
 
  /**************************************************************************/

  {
    unsigned long numberOfClasses = kernel.number_of_classes();
  
    test_start_msg(countOfTests) << "Iterator over " << numberOfClasses << " classes...";
  
    const OksClass::Map & classes = kernel.classes();

    {
      size_t numberOfRepet = performance;
    
      TIME_TEST_START(numberOfRepet)
     
      for(OksClass::Map::const_iterator ic = classes.begin(); ic != classes.end(); ++ic) {
	OksClass *c = ic->second;
	if(!c) std::cerr << "error in iterate-classes test\n";
      }

      TIME_TEST_STOP("iterate-classes")
    }

    table.back()->t /= numberOfClasses;
  }

  /**************************************************************************/

  {
    unsigned long numberOfObjects = EnvironmentClass->number_of_objects();
  
    test_start_msg(countOfTests) << "Iterator over " << numberOfObjects
      << " objects of class \'" << EnvironmentClass->get_name() << "\'...";

    const OksObject::Map * env_objs = EnvironmentClass->objects();

    {
      size_t numberOfIterObjsRepet = (1 + ((performance * 3)/numberOfObjects));
    
      TIME_TEST_START(numberOfIterObjsRepet)
    
      for(OksObject::Map::const_iterator io = env_objs->begin(); io != env_objs->end(); ++io) {
	OksObject *o = (*io).second;
	
  	if(!o) std::cerr << "error in iterate-objects-of-class test\n";
      }

      TIME_TEST_STOP("iterate-objects-of-class")
    }

    table.back()->t /= numberOfObjects;
  }

  /**************************************************************************/

  {
    unsigned long numberOfObjects = kernel.number_of_objects();
  
    test_start_msg(countOfTests) << "Iterator over " << numberOfObjects << " objects...";

    const OksObject::Set & all_objs = kernel.objects();

    {
      size_t numberOfIterObjsRepet = (1 + ((performance * 5)/numberOfObjects));
    
      TIME_TEST_START(numberOfIterObjsRepet)
    
        for(OksObject::Set::const_iterator oi = all_objs.begin(); oi != all_objs.end(); ++oi) {
          OksObject *o = *oi;
          if(!o) std::cerr << "error in iterate-all-objects test\n";
        }

      TIME_TEST_STOP("iterate-all-objects")
    }
  
    table.back()->t /= numberOfObjects;
  }

  /**************************************************************************/

  OksAttribute		* aDefPriority = ProgramClass->find_attribute("DefaultPriority");
  OksData		* d_low = new OksData((int32_t)0);
  OksData               * d_high = new OksData((int32_t)0);
  OksAndExpression	* and_q = new OksAndExpression();
  OksComparator		* cLow = new OksComparator(aDefPriority, d_low, OksQuery::greater_or_equal_cmp);
  OksComparator		* cHight = new OksComparator(aDefPriority, d_high, OksQuery::less_cmp);
  and_q->add(cLow);
  and_q->add(cHight);

  OksQuery q(false, and_q);

  std::list<OksObject *> * dlist = 0;

  unsigned long numberOfProgramObjs = ProgramClass->number_of_objects();
  unsigned long numberOfFoundObjs = 0;

  unsigned long numberOfQueryRepet = (1 + ((performance * 2) / numberOfProgramObjs));

  /**************************************************************************/

	//
	// Set query range to 1%
	//

  d_high->Set((int32_t)((1<<15)-1)/100);

  test_start_msg(countOfTests)
    << " Query to find with range 1% from " << numberOfProgramObjs << " objects...";
  
  TIME_TEST_START(numberOfQueryRepet)

    dlist = ProgramClass->execute_query(&q);
    numberOfFoundObjs = (dlist ? dlist->size() : 0);
    delete dlist;
    dlist = 0;

  TIME_TEST_STOP("query1")

  std::cout << " - found " << numberOfFoundObjs << " objects\n";
  
  /**************************************************************************/

	//
	// Set query range to 10%
	//

  d_high->Set((int32_t)((1<<15)-1)/10);

  test_start_msg(countOfTests) << "Query to find with range 10% from " << numberOfProgramObjs << " objects...";
  
  TIME_TEST_START(numberOfQueryRepet)

    dlist = ProgramClass->execute_query(&q);
    numberOfFoundObjs = (dlist ? dlist->size() : 0);
    delete dlist;
    dlist = 0;

  TIME_TEST_STOP("query10")

  std::cout << " - found " << numberOfFoundObjs << " objects\n";
 
  /**************************************************************************/

	//
	// Set query range to 50%
	//

  d_high->Set((int32_t)((1<<15)-1)/2);

  test_start_msg(countOfTests) << "Query to find with range 50% from " << numberOfProgramObjs << " objects...";
  
  TIME_TEST_START(numberOfQueryRepet)

    dlist = ProgramClass->execute_query(&q);
    numberOfFoundObjs = (dlist ? dlist->size() : 0);
    delete dlist;
    dlist = 0;
  
  TIME_TEST_STOP("query50")

  std::cout << " - found " << numberOfFoundObjs << " objects\n";

  /**************************************************************************/

    //
    // Build index to use it in FAST queries
    //

  {
    OksIndex *index = 0;

    test_start_msg(countOfTests) << "Build index\n";
    TIME_TEST_START(1)

      index = new OksIndex(ProgramClass, aDefPriority);

    TIME_TEST_STOP("build index")

    numberOfQueryRepet = (unsigned long)((double)(performance * 2) / log((double)numberOfProgramObjs)) + 1;
#ifdef __hpux__
    numberOfQueryRepet = numberOfQueryRepet / 3;
#endif


      // set query range to 1%

    {
      d_high->Set((int32_t)((1<<15)-1)/100);

      test_start_msg(countOfTests) << "Query with index to find with range 1% from " << numberOfProgramObjs << " objects...";

      TIME_TEST_START(numberOfQueryRepet)

        dlist = index->FindLessAndGreatEqual(d_high, d_low);
        numberOfFoundObjs = (dlist ? dlist->size() : 0);
        delete dlist;
        dlist = 0;

      TIME_TEST_STOP("query1-with-index")

      std::cout << " - found " << numberOfFoundObjs << " objects\n";
    }


      // set query range to 10%

    {
      d_high->Set((int32_t)((1<<15)-1)/10);

      test_start_msg(countOfTests) << "Query with index to find with range 10% from " << numberOfProgramObjs << " objects...";

      numberOfQueryRepet = (numberOfQueryRepet / 5) + 1;

      TIME_TEST_START(numberOfQueryRepet)

        dlist = index->FindLessAndGreatEqual(d_high, d_low);
        numberOfFoundObjs = (dlist ? dlist->size() : 0);
        delete dlist;
        dlist = 0;

      TIME_TEST_STOP("query10-with-index")

      std::cout << " - found " << numberOfFoundObjs << " objects\n";
    }


      // set query range to 50%

    {
      d_high->Set((int32_t)((1<<15)-1)/2);

      test_start_msg(countOfTests) << "Query with index to find with range 50% from " << numberOfProgramObjs << " objects...";

      numberOfQueryRepet = (numberOfQueryRepet / 7) + 1;

#ifdef __hpux__
      numberOfQueryRepet = numberOfQueryRepet / 4 + 1;
#endif

      TIME_TEST_START(numberOfQueryRepet)

        dlist = index->FindLessAndGreatEqual(d_high, d_low);
        numberOfFoundObjs = (dlist ? dlist->size() : 0);
        delete dlist;
        dlist = 0;

      TIME_TEST_STOP("query50-with-index")

      std::cout << " - found " << numberOfFoundObjs << " objects\n";
    }


       // delete index: end of query tests

    delete index;
  }
  
  /**************************************************************************/

  {
    test_start_msg(countOfTests, "Scan the database contents");
  
    std::ofstream ofs("/dev/null"); 
  
    TIME_TEST_START(1)
  
      ofs << kernel;

    TIME_TEST_STOP("kernel_dump")
  }

  /**************************************************************************/
  
  TIME_TEST_START(1)

    kernel.close_all_data();

    test_start_msg(countOfTests, "Close data file");

  TIME_TEST_STOP("close_data2")

  /**************************************************************************/

  test_start_msg(countOfTests, "Close schema file");
  std::cout << std::endl;

  while(!table.empty()) {
    CallTimeInfo * cti = table.front();
    table.pop_front();
    std::cout << *cti;
    delete cti;
  }

  TIME_TEST_START(1)

  kernel.close_schema(schema_h);

  TIME_TEST_STOP("close_schema")

  } /* Destruct kernel */

  /**************************************************************************/

  std::cout << std::endl;

  while(!table.empty()) {
    CallTimeInfo * cti = table.front();
    table.pop_front();
    std::cout << *cti;
    delete cti;
  }

  return 0;
}
 
