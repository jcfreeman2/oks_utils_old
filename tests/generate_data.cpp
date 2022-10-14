#include <stdlib.h>
#include <stdio.h>

#include <chrono>
#include <fstream>

#include <oks/kernel.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/attribute.h>
#include <oks/relationship.h>


static OksClass*
findClassAndReport(const OksKernel& kernel, const std::string & name)
{
  OksClass *c = kernel.find_class(name);
  
  if(!c) {
    Oks::error_msg("3") << "Can`t find class \"" << name << "\", exiting ...\n";
    exit(3);
  }
  
  return c;
}


static void
createInstances(
  unsigned long n,		// Number of instances
  const char *instance_prefix,	// Prefix for instance id
  OksObject **objs,		// Array of instances
  OksClass *cl			// Pointer to class
)
{
  long j;
 
  std::cout << "Create " << n << " instances of " << cl->get_name() << " class...\n";

  for(j=0; j<(int)n; j++) {
    char buf[32];
    sprintf(buf, "%s%ld", instance_prefix, j);

    objs[j] = new OksObject(cl, buf);
  }
}

int main(int argc, char **argv)
{
  unsigned long	totalNumberOfObjects = 0L;
  unsigned long	numberOfLinksPerSW = 0L;
  unsigned long	numberOfSWObjects = 0L;
  unsigned long	numberOfPrograms = 0L;
  unsigned long	numberOfSWResources = 0L;
  unsigned long	numberOfProcess = 0L;
  unsigned long	numberOfComputers = 0L;
  unsigned long	numberOfApplications = 0L;
  unsigned long	numberOfEnvironment = 0L;

  bool doNotBind = false;
  bool fastExit = false;	// if true do not call OksKernel destructor

  const char *schemaFile = 0;
  const char *dataFile = 0;
 
  for(int m = 1; m < argc; m++) {
    if     (!strcmp(argv[m], "-nbind"))     doNotBind = true;
    else if(!strcmp(argv[m], "-fast-exit")) fastExit = true;
    else if(!strcmp(argv[m], "-n"))         totalNumberOfObjects = atol(argv[++m]);
    else if(!strcmp(argv[m], "-nlinks"))    numberOfLinksPerSW = atol(argv[++m]);
    else if(!strcmp(argv[m], "-schema"))    schemaFile = argv[++m];
    else if(!strcmp(argv[m], "-data"))      dataFile = argv[++m];
    else
      Oks::warning_msg("data-generator")
        << "Unknown parameter: \"" << argv[m] << "\"\n";
  }

  if(!totalNumberOfObjects) totalNumberOfObjects = 2000;
  if(!numberOfLinksPerSW) numberOfLinksPerSW = 40;
  
  if(numberOfLinksPerSW < 10) {
    Oks::warning_msg("data-generator")
      << "Number of links per sw object can not be less 10, set minimal\n";

    numberOfLinksPerSW = 10;
  }

  numberOfSWObjects = totalNumberOfObjects / numberOfLinksPerSW;
  numberOfPrograms = (numberOfSWObjects * 5 * numberOfLinksPerSW) / 40;
  numberOfSWResources = (numberOfSWObjects * 2 * numberOfLinksPerSW) / 40;
  numberOfProcess = (numberOfSWObjects * 8 * numberOfLinksPerSW) / 40;
  numberOfComputers = (numberOfSWObjects * 4 * numberOfLinksPerSW) / 40;
  numberOfApplications = numberOfProcess;
  numberOfEnvironment = totalNumberOfObjects - numberOfSWObjects - numberOfPrograms -
  			numberOfSWResources - numberOfProcess - numberOfComputers -
  			numberOfApplications;


  if(!schemaFile) schemaFile = PATH_TO_SCHEMA;

  if(!dataFile) {
    static char buf[132];
    dataFile = buf;
	
    sprintf(buf, "data.%lu.%lu", totalNumberOfObjects, numberOfLinksPerSW);
  }


  OksKernel kernel;
  OksFile * new_fp = 0;

  try {
    OksFile * schema_fp = kernel.load_schema(schemaFile);
    new_fp = kernel.new_data(dataFile);
std::cout << "ADD INCLUDE FILE \"" << schema_fp->get_full_file_name() << '\"' << std::endl;
    new_fp->add_include_file(schema_fp->get_full_file_name());
  }
  catch (oks::exception & ex) {
    Oks::error_msg("1") << "Caught OKS exception: " << ex.what() << std::endl;
    return 1;
  }


    //
    // Test that schema file contains necessary classes
    //

  OksClass *SWObjectClass    = findClassAndReport(kernel, "SW_Object");
  OksClass *ProgramClass     = findClassAndReport(kernel, "Program");
  OksClass *SWResourceClass  = findClassAndReport(kernel, "SW_Resource");
  OksClass *ProcessClass     = findClassAndReport(kernel, "Process");
  OksClass *ComputerClass    = findClassAndReport(kernel, "Computer");
  OksClass *ApplicationClass = findClassAndReport(kernel, "Application");
  OksClass *EnvironmentClass = findClassAndReport(kernel, "Environment");


    //
    // Allocate space for arrays which contain instances of objects
    //

  OksObject **SWObjectObjects    = new OksObject * [numberOfSWObjects];
  OksObject **ProgramObjects     = new OksObject * [numberOfPrograms];
  OksObject **SWResourceObjects  = new OksObject * [numberOfSWResources];
  OksObject **ProcessObjects     = new OksObject * [numberOfProcess];
  OksObject **ComputerObjects    = new OksObject * [numberOfComputers];
  OksObject **ApplicationObjects = new OksObject * [numberOfApplications];
  OksObject **EnvironmentObjects = new OksObject * [numberOfEnvironment];


    //
    // Create instances of objects
    //

  createInstances(numberOfSWObjects, "o_", SWObjectObjects, SWObjectClass);
  createInstances(numberOfPrograms, "p_", ProgramObjects, ProgramClass);
  createInstances(numberOfSWResources, "r_", SWResourceObjects, SWResourceClass);
  createInstances(numberOfProcess, "pr_", ProcessObjects, ProcessClass);
  createInstances(numberOfComputers, "c_", ComputerObjects, ComputerClass);
  createInstances(numberOfApplications, "a_", ApplicationObjects, ApplicationClass);
  createInstances(numberOfEnvironment, "e_", EnvironmentObjects, EnvironmentClass);


    //
    // Use 'OksDataInfo' instead of string name when
    // access attributes and relationships to
    // improve performance
    //

  OksDataInfo *ApplicationTimeout_odi		= ApplicationClass->data_info("InitTimeout");
  OksDataInfo *ApplicationNeedsEnvironment_odi	= ApplicationClass->data_info("NeedsEnvironment");
  OksDataInfo *ApplicationRunsOn_odi		= ApplicationClass->data_info("RunsOn");
  OksDataInfo *ApplicationExecutedAs_odi	= ApplicationClass->data_info("ExecutedAs");
  OksDataInfo *ApplicationSWObject_odi		= ApplicationClass->data_info("SWObject");

  OksDataInfo *ComputerOsType_odi		= ComputerClass->data_info("OsType");
  OksDataInfo *ComputerExecutes_odi		= ComputerClass->data_info("Executes");
  
  OksDataInfo *ProcessStartedFrom_odi		= ProcessClass->data_info("StartedFrom");
  OksDataInfo *ProcessRunsOn_odi		= ProcessClass->data_info("RunsOn");
  OksDataInfo *ProcessRanBy_odi			= ProcessClass->data_info("RanBy");
  OksDataInfo *ProcessHoldsResource_odi		= ProcessClass->data_info("HoldsResource");

  OksDataInfo *ProgramOsType_odi		= ProgramClass->data_info("OsType");
  OksDataInfo *ProgramDescribedBy_odi		= ProgramClass->data_info("DescribedBy");
  OksDataInfo *ProgramNeedsEnvironment_odi	= ProgramClass->data_info("NeedsEnvironment");
  OksDataInfo *ProgramExecutableFile_odi	= ProgramClass->data_info("ExecutableFile");
  OksDataInfo *ProgramDefaultParameters_odi	= ProgramClass->data_info("DefaultParameters");
  OksDataInfo *ProgramDefaultPriority_odi	= ProgramClass->data_info("DefaultPriority");
  OksDataInfo *ProgramDefaultPrivileges_odi	= ProgramClass->data_info("DefaultPrivileges");
  
  OksDataInfo *SWObjectImplementedBy_odi	= SWObjectClass->data_info("ImplementedBy");
  OksDataInfo *SWObjectNeedsEnvironment_odi	= SWObjectClass->data_info("NeedsEnvironment");
  OksDataInfo *SWObjectNeedsResources_odi	= SWObjectClass->data_info("NeedsResources");
  OksDataInfo *SWObjectName_odi			= SWObjectClass->data_info("Name");
  OksDataInfo *SWObjectDescription_odi		= SWObjectClass->data_info("Description");
  OksDataInfo *SWObjectAuthors_odi		= SWObjectClass->data_info("Authors");
  OksDataInfo *SWObjectHelpLink_odi		= SWObjectClass->data_info("HelpLink");
  OksDataInfo *SWObjectDefaultParameter_odi	= SWObjectClass->data_info("DefaultParameter");
  OksDataInfo *SWObjectDefaultPriority_odi	= SWObjectClass->data_info("DefaultPriority");
  OksDataInfo *SWObjectDefaultPrivileges_odi	= SWObjectClass->data_info("DefaultPrivileges");

  OksDataInfo *SWResourceAllocatedBy_odi	= SWResourceClass->data_info("AllocatedBy");


    // Set Application Timeouts

  {
    std::cout << "Set applications timeouts...\n";
  
    for(int i=0; i<(int)numberOfApplications; ++i) {
      OksData *d_timeout(ApplicationObjects[i]->GetAttributeValue(ApplicationTimeout_odi));
      d_timeout->data.U32_INT = (uint32_t)(rand() % 256);
    }
  }

    // set operating system type for instances of Program and Computer classes

  {
    std::cout << "Set operating system types...\n";
  
    OksAttribute *a = ProgramClass->find_attribute("OsType");
	
    for(int i=0; i<(int)numberOfPrograms; i++) {
      OksData d_osType;
	
      switch((unsigned int)(rand() % 4)) {
        case 0: d_osType.SetValues("lynx", a);    break;
        case 1: d_osType.SetValues("wnt", a);     break;
        case 2: d_osType.SetValues("solaris", a); break;
        case 3: d_osType.SetValues("hpux", a);    break;
      }

      try {
        ProgramObjects[i]->SetAttributeValue(ProgramOsType_odi, &d_osType);
      }
      catch(oks::exception& ex) {
        Oks::error_msg("10") << "Failed to set attribute value of " << ProgramObjects[i] << ": " << ex.what() << std::endl;
	return 10;
      }
    }

    for(int i=0; i<(int)numberOfComputers; i++) {
      OksData d_osType;

      switch((unsigned int)(rand() % 4)) {
        case 0: d_osType.SetValues("lynx", a);    break;
        case 1: d_osType.SetValues("wnt", a);     break;
        case 2: d_osType.SetValues("solaris", a); break;
        case 3: d_osType.SetValues("hpux", a);    break;
      }

      try {
        ComputerObjects[i]->SetAttributeValue(ComputerOsType_odi, &d_osType);
      }
      catch(oks::exception& ex) {
        Oks::error_msg("11") << "Failed to set attribute value of " << ComputerObjects[i] << ": " << ex.what() << std::endl;
	return 11;
      }
    }
  }


  if(!doNotBind) {
    std::cout << "Bind instances..." << std::endl;
	
    register int i, j, j2;
    OksData *d;
	
    for(i=0; i<(int)numberOfPrograms; ++i) {
      j = (
        (i<(int)numberOfSWObjects)
          ? i
          : (rand() % numberOfSWObjects)
      );

      ProgramObjects[i]->SetRelationshipValue(ProgramDescribedBy_odi, SWObjectObjects[j]);
      SWObjectObjects[j]->AddRelationshipValue(SWObjectImplementedBy_odi, ProgramObjects[i]);

      if(!((i+1) % 2000)) {std::cout << '*' << ' '; std::cout.flush();}
    }

    for(i=0; i<(int)numberOfProcess; ++i) {
      j = rand() % numberOfPrograms;

      ProcessObjects[i]->SetRelationshipValue(ProcessStartedFrom_odi, ProgramObjects[j]);
      OksData *d_osType(ProgramObjects[j]->GetAttributeValue(ProgramOsType_odi));

      for(;;) {
        j2 = rand() % numberOfComputers;
        OksData *d_osType2(ComputerObjects[j2]->GetAttributeValue(ComputerOsType_odi));
        if(*d_osType2 == *d_osType) break;
      }

      ComputerObjects[j2]->AddRelationshipValue(ComputerExecutes_odi, ProcessObjects[i]);
      ProcessObjects[i]->SetRelationshipValue(ProcessRunsOn_odi, ComputerObjects[j2]);
      ApplicationObjects[i]->SetRelationshipValue(ApplicationRunsOn_odi, ComputerObjects[j2]);

      ProcessObjects[i]->SetRelationshipValue(ProcessRanBy_odi, ApplicationObjects[i]);
      ApplicationObjects[i]->SetRelationshipValue(ApplicationExecutedAs_odi, ProcessObjects[i]);

      d = ProgramObjects[j]->GetRelationshipValue(ProgramDescribedBy_odi);
      ApplicationObjects[i]->SetRelationshipValue(ApplicationSWObject_odi, d->data.OBJECT);

      j = rand() % numberOfSWResources;
		
      if(j == (int)numberOfSWResources) j--;

      ProcessObjects[i]->AddRelationshipValue(ProcessHoldsResource_odi, SWResourceObjects[j]);
      SWResourceObjects[j]->AddRelationshipValue(SWResourceAllocatedBy_odi, ProcessObjects[i]);
      d->data.OBJECT->AddRelationshipValue(SWObjectNeedsResources_odi, SWResourceObjects[j]);

      if(!((i+1) % 1000)) {std::cout << "@ "; std::cout.flush();}
    }

    for(i=0; i<(int)numberOfEnvironment; ++i) {
      j = rand() % 100;
	
      if(j<25 || !(j%10))		/* needs for sw_object (32%) */
        SWObjectObjects[(rand() % numberOfSWObjects)]->AddRelationshipValue(SWObjectNeedsEnvironment_odi, EnvironmentObjects[i]);
      if((j>25 && j<50) || !(j%8))	/* needs for program  (34%) */
        ProgramObjects[(rand() % numberOfPrograms)]->AddRelationshipValue(ProgramNeedsEnvironment_odi, EnvironmentObjects[i]);
      else if(j>50 || !(j%7))		/* needs for application (57%) */
        ApplicationObjects[(rand() % numberOfApplications)]->AddRelationshipValue(ApplicationNeedsEnvironment_odi, EnvironmentObjects[i]);

      if(!((i+1) % 2500)) {std::cout << '#' << ' '; std::cout.flush();}
    }

    if(numberOfEnvironment > 2500 || numberOfProcess > 1000 || numberOfPrograms > 2000) std::cout << std::endl;
  }

  register int i;
  
  std::cout << "Set attributes...\n";
  
  for(i=0; i<(int)numberOfSWObjects; i++) {
    char buf[128];
  	
    sprintf(buf, "a server #%ld", (unsigned long)rand());
    OksData d_Name(buf);

    OksData d_Desc("This is a test server");
	
    OksData d_Auth(new OksData::List()); 
    register int j = (int)(rand() % 7) + 1;
    while(j) {
      sprintf(buf, "atdsoft #%d", j--);
      d_Auth.data.LIST->push_back(new OksData(buf));
    }

    sprintf(buf, "http://atddoc.cern.ch/Atlas/online/sw/%d.html", i);
    OksData d_Help(buf);
	
    sprintf(buf, "-test -n %d", i);
    OksData d_Prms(buf);
	
    int32_t priority = rand() % ((1<<15)-1);
    OksData d_Prir(priority);
	
    OksData d_Priv("no");
	
    SWObjectObjects[i]->SetAttributeValue(SWObjectName_odi, &d_Name);
    SWObjectObjects[i]->SetAttributeValue(SWObjectDescription_odi, &d_Desc);
    SWObjectObjects[i]->SetAttributeValue(SWObjectAuthors_odi, &d_Auth);
    SWObjectObjects[i]->SetAttributeValue(SWObjectHelpLink_odi, &d_Help);
    SWObjectObjects[i]->SetAttributeValue(SWObjectDefaultParameter_odi, &d_Prms);
    SWObjectObjects[i]->SetAttributeValue(SWObjectDefaultPriority_odi, &d_Prir);
    SWObjectObjects[i]->SetAttributeValue(SWObjectDefaultPrivileges_odi, &d_Priv);
  }


    // free list of SWObject class instances

  delete[] SWObjectObjects;
  

  for(i=0; i<(int)numberOfPrograms; i++) {
    char buf[128];
    OksData *d_osType(ProgramObjects[i]->GetAttributeValue("OsType"));
 
    sprintf(
      buf,
      "/usr/local/dist/last/installed/g++/%s/bin/s%d",
      d_osType->data.ENUMERATION->c_str(),
      rand()
    );
    OksData d_ExecutableFile(buf);
	
    sprintf(buf, "-test -n %d", i);
    OksData d_Prms(buf);
	
    int32_t priority = rand() % ((1<<15)-1);
    OksData d_Prir(priority);

    OksData d_Priv("no");
	
    ProgramObjects[i]->SetAttributeValue(ProgramExecutableFile_odi, &d_ExecutableFile);
    ProgramObjects[i]->SetAttributeValue(ProgramDefaultParameters_odi, &d_Prms);
    ProgramObjects[i]->SetAttributeValue(ProgramDefaultPriority_odi, &d_Prir);
    ProgramObjects[i]->SetAttributeValue(ProgramDefaultPrivileges_odi, &d_Priv);
  }


    // free list of Program class instances

  delete[] ProgramObjects;


    // free list of other class instances

  delete[] SWResourceObjects;
  delete[] ProcessObjects;
  delete[] ComputerObjects;
  delete[] ApplicationObjects;
  delete[] EnvironmentObjects;

  auto tp = std::chrono::steady_clock::now();

  try {
    kernel.save_data(new_fp, true);
  }
  catch (oks::exception & e) {
    Oks::error_msg("4") << "cannot save oks database data file \"" << dataFile << "\": " << e.what() << std::endl;
    return 4;
  }
  catch (...) {
    Oks::error_msg("4") << "cannot save oks database data file \"" << dataFile << '\"' << std::endl;
    return 4;
  }

  std::cout << "Time to save data is " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-tp).count() / 1000. << " ms" << std::endl;


  if(fastExit) {
    new_fp->unlock();
    exit(0);
  }

  return 0;
}
