/**
 *  \file oks_diff_data.cpp
 *
 *  This file is part of the OKS package.
 *  Author: <Igor.Soloviev@cern.ch>
 *
 *  This file contains the implementation of the OKS application to show
 *  differences between two data files.
 *
 *  Usage:
 *
 *    oks_diff_data [--attributes] [--relationships] [--composite-parents]
 *                  [-all] [--help] [--version] [--schema-files schema_file(s)]
 *                  [--class-name name_of_class [--object-id id_of_object]]
 *                  --data-file1 data_file1 --data-file2 data_file2
 *
 *    Options (if two objects differed):
 *      \param --attributes                produce a listing of differences for object's attributes
 *      \param --relationships             produce a listing of differences for object's relationships
 *      \param --composite-parents         produce a listing of object's composite parents
 *      \param --all                       the same as '--attributes --relationships --composite-parents'
 *
 *    General Options/Arguments:
 *      \param --class-name class-name     optional name of class which objects to be checked\n"
 *      \param --object-id object-id       optional id to check only this object (only used with --class-name)\n"
 *      \param --schema-files schema-file  optional list of schema files
 *      \param --data-file1 data-file      first compared datafile
 *      \param --data-file2 data-file      second compared datafile
 *      \param --version                   print version
 *      \param --verbose                   verbose mode
 *      \param --help                      print help text
 */


#include <iostream>

#include <set>

#include "oks/kernel.hpp"
#include "oks/class.hpp"
#include "oks/object.hpp"
#include "oks/attribute.hpp"
#include "oks/relationship.hpp"


std::string appTitle;


static void
printRecursion(int level)
{
  while(level--) std::cout << "  ";
}
  
static void
ReportCompositeParents(OksObject *o, int recursionLevel = -1)
{
  static std::set<OksRCR *, std::less<OksRCR *> > rcr_set;
  
  recursionLevel++;
  
  const std::list<OksRCR *> * rcr_list = o->reverse_composite_rels();
  
  if(rcr_list) {
    for(std::list<OksRCR *>::const_iterator i = rcr_list->begin(); i != rcr_list->end(); ++i) {
      OksRCR *rcr = *i;
		
      printRecursion(recursionLevel);
  		
      std::cout << "     " << rcr->obj << " ==\\" << rcr->relationship->get_name()
                << "\\==> " << o << std::endl;
	
      if(!rcr_set.empty() && rcr_set.find(rcr) != rcr_set.end()) {
        printRecursion(recursionLevel);
        std::cout << "     The composite parent(s) of " << rcr->obj
	          << " were reported in this loop (circular references)\n";
      }
      else {
        rcr_set.insert(rcr);
        ReportCompositeParents(rcr->obj, recursionLevel);
        rcr_set.erase(rcr);
      }
    }
  }
  else {
     printRecursion(recursionLevel);
     std::cout << "     " << o << " has no composite parents\n";
  }
  
  recursionLevel--;
}

static void
printShortUsage(const char *appName, std::ostream& s)
{
  s << appTitle << "\n"
	"Usage: " << appName << " [-a] [-r] [-p] [-all] [-h] [-v] [-c class [-o object_id]] "
	                        "[-s schema_file(s)] -d1 data_file1 -d2 data_file2\n"
	"\n"
	"  Options (if two objects differed):\n"
	"\t-a              produce a listing of differences for object's attributes\n"
	"\t-r              produce a listing of differences for object's relationships\n"
	"\t-p              produce a listing of object's composite parents\n"
	"\t-all            the same as \'-a -r -p\'\n"
	"\n"
	"  General Options/Arguments:\n"
	"\t-c class-name   optional name of class which objects to be checked\n"
	"\t-o object-id    optional id to check only this object (only used with -c)\n"
	"\t-s schema-file  optional list of schema files\n"
	"\t-d1 data-file   first compared datafile\n"
	"\t-d2 data-file   second compared datafile\n"
	"\t-v              print version\n"
	"\t-b              verbose mode\n"
	"\t-h              print this text\n"
	"\n";
}


static void
printUsage(const char *appName, std::ostream& s)
{
  s << appTitle << "\n"
	"Usage: " << appName << " [--attributes] [--relationships] [--composite-parents] [--all] [--help] [--version]"
                                " [--class-name name_of_class [--object-id id_of_object]]\n"
                                " [--schema-files schema_file(s)] --data-file1 data_file1 --data-file2 data_file2\n"
	"\n"
	"  Options (if two objects differed):\n"
	"\t--attributes                produce a listing of differences for object's attributes\n"
	"\t--relationships             produce a listing of differences for object's relationships\n"
	"\t--composite-parents         produce a listing of object's composite parents\n"
	"\t-all                        the same as \'--attributes --relationships --composite-parents\'\n"
	"\n"
	"  General Options/Arguments:\n"
	"\t--class-name class-name     optional name of class which objects to be checked\n"
	"\t--object-id object-id       optional id to check only this object (only used with --class-name)\n"
	"\t--schema-files schema-file  optional list of schema files\n"
	"\t--data-file1 data-file      first compared datafile\n"
	"\t--data-file2 data-file      second compared datafile\n"
	"\t--version                   print version\n"
	"\t--verbose                   verbose mode\n"
	"\t--help                      print this text\n"
	"\n";
}


int
main(int argc, const char * argv[])
{
  appTitle = "OKS data files comparer. OKS kernel version ";
  appTitle += OksKernel::GetVersion();

  bool printAttributeDifferences = false;
  bool printRelationshipDifferences = false;
  bool printCompositeParentsMode = false;
  bool verboseMode = false;

  const char * appName = "oks_diff_data";

  if(argc == 1) {
    printUsage(appName, std::cerr);
    return 1;
  }


    // allocate enough space for list of schema files

  const char ** schemaFiles = new const char * [argc];
  schemaFiles[0] = 0;

  const char * dataFile1 = 0;
  const char * dataFile2 = 0;
  
  const char * class_name = 0;
  const char * object_id = 0;
  
  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-a") || !strcmp(argv[i], "--attributes"))
      printAttributeDifferences = true;
    else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--relationships"))
      printRelationshipDifferences = true;
    else if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--composite-parents"))
      printCompositeParentsMode = true;
    else if(!strcmp(argv[i], "-all") || !strcmp(argv[i], "--all")) {
      printAttributeDifferences = true;
      printRelationshipDifferences = true;
      printCompositeParentsMode = true;
    }
    else if(!strcmp(argv[i], "-b") || !strcmp(argv[i], "--verbose"))
      verboseMode = true;
    else if(!strcmp(argv[i], "-h")) {
      printShortUsage(appName, std::cout);		
      return 0;
    }
    else if(!strcmp(argv[i], "--help")) {
      printUsage(appName, std::cout);		
      return 0;
    }
    else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
      std::cout << appTitle << std::endl;		
      return 0;
    }
    else if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "--schema-files")) {
      for(int j = 0; j < argc - i - 1; ++j) {
        if(argv[i+1+j][0] != '-') schemaFiles[j] = argv[i+1+j];
	else {
	  schemaFiles[j]=0;
	  i+=j;
	  break;
	}
      }
    }
    else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--class-name")) {
      if(argv[i+1][0] != '-') class_name=argv[++i];
    }
    else if(!strcmp(argv[i], "-o") || !strcmp(argv[i], "--object-id")) {
      if(argv[i+1][0] != '-') object_id=argv[++i];
    }
    else if(!strcmp(argv[i], "-d1") || !strcmp(argv[i], "--data-file1")) {
      if(argv[i+1][0] != '-') dataFile1=argv[++i];
    }
    else if(!strcmp(argv[i], "-d2") || !strcmp(argv[i], "--data-file2")) {
      if(argv[i+1][0] != '-') dataFile2=argv[++i];
    }
    else {
      std::cerr << "ERROR: Unknown parameter: \"" << argv[i] << "\"\n\n";
      printUsage(appName, std::cerr);
      return 1;
    }
  }


    // check schema and data files

  if(dataFile1 == 0) {
    std::cerr << "ERROR: First data file is not defined.\n\n";
    printUsage(appName, std::cerr);
    return 1;
  }

  if(dataFile2 == 0) {
    std::cerr << "ERROR: Second data file is not defined.\n\n";
    printUsage(appName, std::cerr);
    return 1;
  }


    // check object id and class name

  if(object_id && !class_name) {
    std::cerr << "ERROR: object ID cannot be used without class name.\n\n";
    printUsage(appName, std::cerr);
    return 1;
  }


  int return_status = 0;
  

    // create two kernels and load the data

  OksKernel kernel1;
  OksKernel kernel2;

  try {

    if(!verboseMode) {
      kernel1.set_silence_mode(true);
      kernel2.set_silence_mode(true);
    }

    for(int i=0; schemaFiles[i] != 0; ++i) {
      const char *s_file = schemaFiles[i];

      kernel1.load_schema(s_file);
      kernel2.load_schema(s_file);
    }

    kernel1.load_data(dataFile1);
    kernel2.load_data(dataFile2);

    const OksClass::Map& classes = kernel1.classes();
    OksClass::Map::const_iterator class_iterator = classes.begin();

    if(classes.empty())  {
      std::cerr << "ERROR: No classes were found in loaded schema file(s), exiting ...\n";
      return 4;
    }

    std::cout << std::endl;

    int classDiffsCount = 0;

    OksClass *c1, *c2;
    for(;class_iterator != classes.end(); ++class_iterator) {
      c1 = class_iterator->second;
      std::string className(c1->get_name());

      if(class_name && className != class_name) {
        continue;
      }

      if(verboseMode) classDiffsCount = 0;

      if(verboseMode) {
        std::cout << "TESTING IN CLASS \"" << className << "\"... ";
        std::cout.flush();
      }

      if(!(c2 = kernel2.find_class(className))) {
        if(verboseMode && !classDiffsCount) std::cout << std::endl;
        std::cout << " DIFFERENCE " << ++classDiffsCount << "."
                     " Database \'" << dataFile2 << "\' does not have class \"" << className << "\", skip testing ...\n";
        continue;
      }

      if(*c1 != *c2) {
        if(verboseMode && !classDiffsCount) std::cout << std::endl;
        std::cout << " DIFFERENCE " << ++classDiffsCount << "."
                     " Class \"" << className << "\" is different in \"" << dataFile1 << "\" and \"" << dataFile2 << "\", skip testing ...\n";
        continue;
      }

      const OksObject::Map * objects1 = c1->objects();
      const OksObject::Map * objects2 = c2->objects();
      OksObject::Map::const_iterator object_iterator = objects1->begin();

      for(;object_iterator != objects1->end(); ++object_iterator) {
        const std::string * objUID = (*object_iterator).first;
        OksObject * o1 = (*object_iterator).second;

        if(object_id && o1->GetId() != object_id) {
          continue;
        }

        OksObject * o2 = c2->get_object(objUID);

        if(!o2) {
          if(verboseMode && !classDiffsCount) std::cout << std::endl;
          std::cout << " DIFFERENCE " << ++classDiffsCount << "."
                       " There is no object " << o1 << " in the \"" << dataFile2 << "\" database file.\n";
          continue;
        }

        if(*o1 == *o2) continue;

        if(verboseMode && !classDiffsCount) std::cout << std::endl;
        std::cout << " DIFFERENCE " << ++classDiffsCount << ". The OBJECTS " << o1 << " differed:\n";

        int objDiffsCount = 0;


        const std::list<OksAttribute *> * alist = c1->all_attributes();
        const std::list<OksRelationship *> * rlist = c1->all_relationships();

        if(alist) {
          for(std::list<OksAttribute *>::const_iterator ai = alist->begin(); ai != alist->end(); ++ai) {
            OksAttribute * a = *ai;
            OksData * d1(o1->GetAttributeValue(a->get_name()));
            OksData * d2(o2->GetAttributeValue(a->get_name()));

            if(!(*d1 == *d2)) {
              ++objDiffsCount;

              if(printAttributeDifferences) {
                std::cout << "   " << classDiffsCount << "." << objDiffsCount
                          << " The ATTRIBUTE \"" << a->get_name() << "\" values differed:\n"
                             "    the value in the FILE \"" << dataFile1 << "\" is: " << *d1 << "\n"
                             "    the value in the FILE \"" << dataFile2 << "\" is: " << *d2 << std::endl;
              }
            }
          }
        }


        if(rlist) {
          for(std::list<OksRelationship *>::const_iterator ri = rlist->begin(); ri != rlist->end(); ++ri) {
            OksRelationship * r = *ri;
            OksData * d1(o1->GetRelationshipValue(r->get_name()));
            OksData * d2(o2->GetRelationshipValue(r->get_name()));

            if(!(*d1 == *d2)) {
              ++objDiffsCount;
	 			
              if(printRelationshipDifferences) {
                std::cout << "   " << classDiffsCount << '.' << objDiffsCount
                          << " The RELATIONSHIP \"" << r->get_name() << "\" values differed:\n"
                             "    the value in the FILE \"" << dataFile1 << "\" is: " << *d1 << "\n"
                             "    the value in the FILE \"" << dataFile2 << "\" is: " << *d2 << std::endl;
              }
            }
          }	
        }
		
        if(objDiffsCount && printCompositeParentsMode) {
          std::cout << "   COMPOSITE PARENT(S) of the OBJECTS " << o1 << ":\n";
          std::cout << "    In the FILE \"" << dataFile1 << "\"\n"; ReportCompositeParents(o1);
          std::cout << "    In the FILE \"" << dataFile2 << "\"\n"; ReportCompositeParents(o2);
        }
      }


      object_iterator = objects2->begin();

      for(;object_iterator != objects2->end(); ++object_iterator) {
        const std::string * objUID = (*object_iterator).first;
        OksObject * o1 = c1->get_object(objUID);

        if(!o1 && (!object_id || *objUID == object_id)) {
          if(!classDiffsCount) std::cout << std::endl;
          std::cout << " DIFFERENCE " << ++classDiffsCount << "."
                  " There is no object " << (*object_iterator).second << " in the \"" << dataFile1 << "\" database file.\n";
        }
      }

      if(verboseMode && !classDiffsCount) {
        std::cout << " no differences were found\n";
      }
    }
  }

  catch (oks::exception & ex) {
    std::cerr << "Caught oks exception:\n" << ex << std::endl;
    return_status = 10;
  }

  catch (std::exception & e) {
    std::cerr << "Caught standard C++ exception: " << e.what() << std::endl;
    return_status = 10;
  }

  catch (...) {
    std::cerr << "Caught unknown exception" << std::endl;
    return_status = 10;
  }

  delete [] schemaFiles;

  if(verboseMode)
    std::cout << "\nExiting " << appName << "...\n";

  return return_status;
}
