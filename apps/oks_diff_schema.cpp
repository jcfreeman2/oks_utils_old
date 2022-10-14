/**
 *  \file oks_diff_schema.cpp
 *
 *  This file is part of the OKS package.
 *  Author: <Igor.Soloviev@cern.ch>
 *
 *  This file contains the implementation of the OKS application to show
 *  differences between two schema files.
 *
 *  Usage:
 *
 *    oks_diff_schema [-a] [-r] [-m] [-sb] [-all] [-h] [--help] [-v]
 *                    schema_file1 schema_file2
 *
 *    Options (if two classes differed):
 *       \param  -a    produce a listing of detail differences for attributes
 *       \param  -r    produce a listing of detail differences for relationships
 *       \param  -m    produce a listing of detail differences for methods
 *       \param  -sb   produce a listing of all sub classes
 *       \param  -all  the same as -a -r -m -sb
 *
 *    General options:
 *       \param  -v    print version
 *       \param  -h    print this text
 *
 */


#include <fstream>

#include "oks/attribute.hpp"
#include "oks/relationship.hpp"
#include "oks/method.hpp"
#include "oks/kernel.hpp"
#include "oks/class.hpp"


int    localDiffsCount;
int    classDiffsCount;
char * schemaFile1 = 0;
char * schemaFile2 = 0;


static void
ReportSubClasses(OksClass *c)
{
  const OksClass::FList * clist = c->all_sub_classes();

  if(clist && !clist->empty()) {
    for(OksClass::FList::const_iterator i = clist->begin(); i != clist->end(); ++i)
      std::cout << "     " << (*i)->get_name() << std::endl;
  }
  else
    std::cout << "     no subclasses\n";
}

enum {
  __Success__ = 0,
  __StartOfBadStatus__ = 252,
  __BadCommandLine__,
  __CannotReadFile__,
  __ThereAreNoClasses__
};

static void printUsage(std::ostream& s) {
  s << "Usage: oks_diff_schema [-a] [-r] [-m] [-sb] [-all] [-h | --help] [-v] schema_file1 schema_file2\n"
       "Options:\n"
       "    -a           print details of attribute differences\n"
       "    -r           print details of relationship differences\n"
       "    -m           print details of method differences\n"
       "    -sb          print all sub classes\n"
       "    -all         the same as -a -r -m -sb\n"
       "    -v           print version\n"
       "    -h | --help  print this text\n"
       "\n"
       "Description:\n"
       "    Print out differences between two schema files.\n"
       "\n"
       "Return Status:\n"
       "        0 - there are no differences between two schema files\n"
       "      " << __BadCommandLine__ << " - bad command line\n"
       "      " << __CannotReadFile__ << " - cannot load database file(s)\n"
       "      " << __ThereAreNoClasses__ << " - loaded file has no any class\n"
       "   1.." << __StartOfBadStatus__ << " - number of differences (is limited by the max possible value)\n";
}


static void
attributesTest(bool printAttributeDifferences, OksClass *c1, OksClass *c2)
{
  const std::list<OksAttribute *> * alist = c1->direct_attributes();
  OksAttribute *a1, *a2;

  if(alist) {
    for(std::list<OksAttribute *>::const_iterator ai = alist->begin(); ai != alist->end(); ++ai) {
      OksAttribute *a1 = *ai;
      const std::string& attributeName = a1->get_name();

      if( !(a2 = c2->find_attribute(attributeName)) ) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " There is no attribute \"" << attributeName << "\" defined in the \"" << schemaFile2 << "\" database file.\n";

        continue;
      }

      if(!(*a1 == *a2)) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " The ATTRIBUTE \"" << attributeName << "\" differed";

        if(!printAttributeDifferences)
          std::cout << ".\n";
        else {
          int localAttrCount = 0;
	 			
          const char * szTheAttribute	= " The Attribute ";
          const char * szMultiValues	= "multi-values";
          const char * szSingleValues	= "single-value";
          const char * szItCan		= "\" it can ";
          const char * szNot		= "not ";
          const char * szBeNull		= "be null\n";
	 			
          std::cout << ":\n";

          if(a1->get_type() != a2->get_type())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheAttribute << "Types differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << a1->get_type() << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << a2->get_type() << "\"\n";

          if(a1->get_range() != a2->get_range())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheAttribute << "Ranges differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << a1->get_range() << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << a2->get_range() << "\"\n";

          if(a1->get_is_multi_values() != a2->get_is_multi_values())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheAttribute << "Cardinality differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << (a1->get_is_multi_values() ? szMultiValues : szSingleValues) << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << (a2->get_is_multi_values() ? szMultiValues : szSingleValues) << "\"\n";

          if(a1->get_init_value() != a2->get_init_value())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheAttribute << "Initial Values differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << a1->get_init_value() << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << a2->get_init_value() << "\"\n";

          if(a1->get_description() != a2->get_description())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheAttribute << "Descriptions differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << a1->get_description() << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << a2->get_description() << "\"\n";

          if(a1->get_is_no_null() != a2->get_is_no_null())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheAttribute << "Constraints differed: \n"
                      << "      in the FILE \"" << schemaFile1 << szItCan << (a1->get_is_no_null() ? szNot : "") << szBeNull
                      << "      in the FILE \"" << schemaFile2 << szItCan << (a2->get_is_no_null() ? szNot : "") << szBeNull;
        }
      }
    }
  }

  alist = c2->direct_attributes();
	
  if(alist) {
    for(std::list<OksAttribute *>::const_iterator ai = alist->begin(); ai != alist->end(); ++ai) {
      OksAttribute *a2 = *ai;
      const std::string & attributeName = a2->get_name();
      if( !(a1 = c1->find_attribute(attributeName)) ) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " There is no attribute \"" << attributeName << "\" defined in the \"" << schemaFile1 << "\" database file.\n";
        continue;
      }
    }
  }
}


static void
relationshipsTest(bool printRelationshipDifferences, OksClass *c1, OksClass *c2)
{	
  const char *szRELATIONSHIP	= "RELATIONSHIP ";
	
  const std::list<OksRelationship *> * rlist = c1->direct_relationships();
  OksRelationship *r1, *r2;
	
  if(rlist) {
    for(std::list<OksRelationship *>::const_iterator ri = rlist->begin(); ri != rlist->end(); ++ri) {
      OksRelationship *r1 = *ri;
      const std::string & relationshipName = r1->get_name();

      if( !(r2 = c2->find_relationship(relationshipName)) ) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " There is no " << szRELATIONSHIP << "\"" << relationshipName << "\" defined in the \"" << schemaFile2 << "\" database file.\n" ;
	
        continue;
      }

      if(!(*r1 == *r2)) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " The " << szRELATIONSHIP << "\"" << relationshipName << "\" differed";
        if(!printRelationshipDifferences)
          std::cout << ".\n";
        else {
          int localAttrCount = 0;
	 				 			
          const char * szTheRelationship = " The Relationship ";
          const char * szReference	 = " reference\n";
 	  const char * szZero		 = "Zero";
 	  const char * szOne		 = "One";
	  const char * szMany		 = "Many";
          const char * szComposite	 = "composite";
          const char * szWeak		 = "weak";
          const char * szExclusive	 = "exclusive";
          const char * szShared		 = "shared";
          const char * szDependent	 = "dependent";
          const char * szIndependent	 = "independent";


          std::cout << ":\n";

          if(r1->get_type() != r2->get_type())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheRelationship << "Class Types differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << r1->get_type() << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << r2->get_type() << "\"\n";

	  if(r1->get_description() != r2->get_description())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheRelationship << "Descriptions differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << r1->get_description() << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << r2->get_description() << "\"\n";

	  if(r1->get_low_cardinality_constraint() != r2->get_low_cardinality_constraint())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheRelationship << "Low Cardinality Constraint differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << (r1->get_low_cardinality_constraint() == OksRelationship::Zero ? szZero : (r1->get_low_cardinality_constraint() == OksRelationship::One ? szOne : szMany)) << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << (r2->get_low_cardinality_constraint() == OksRelationship::Zero ? szZero : (r2->get_low_cardinality_constraint() == OksRelationship::One ? szOne : szMany)) << "\"\n";

	  if(r1->get_high_cardinality_constraint() != r2->get_high_cardinality_constraint())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheRelationship << "High Cardinality Constraint differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << (r1->get_high_cardinality_constraint() == OksRelationship::Zero ? szZero : (r1->get_high_cardinality_constraint() == OksRelationship::One ? szOne : szMany)) << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << (r2->get_high_cardinality_constraint() == OksRelationship::Zero ? szZero : (r2->get_high_cardinality_constraint() == OksRelationship::One ? szOne : szMany)) << "\"\n";

	  if(r1->get_is_composite() != r2->get_is_composite())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheRelationship << "Composite Types differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is " << (r1->get_is_composite() ? szComposite : szWeak) << szReference
                      << "      in the FILE \"" << schemaFile2 << "\" it is " << (r2->get_is_composite() ? szComposite : szWeak) << szReference;

	  if(r1->get_is_exclusive() != r2->get_is_exclusive())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheRelationship << "Composite Exclusive Types differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is " << (r1->get_is_exclusive() ? szExclusive : szShared) << szReference
                      << "      in the FILE \"" << schemaFile2 << "\" it is " << (r2->get_is_exclusive() ? szExclusive : szShared) << szReference;

	  if(r1->get_is_dependent() != r2->get_is_dependent())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheRelationship << "Composite Dependent Types differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is " << (r1->get_is_dependent() ? szDependent : szIndependent) << szReference
                      << "      in the FILE \"" << schemaFile2 << "\" it is " << (r2->get_is_dependent() ? szDependent : szIndependent) << szReference;
        }
      }
    }
  }
	
	
  rlist = c2->direct_relationships();
	
  if(rlist) {
    for(std::list<OksRelationship *>::const_iterator ri = rlist->begin(); ri != rlist->end(); ++ri) {
      OksRelationship *r2 = *ri;
      const std::string & relationshipName = r2->get_name();

      if( !(r1 = c1->find_relationship(relationshipName)) ) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " There is no " << szRELATIONSHIP << "\"" << relationshipName << "\" defined in the \"" << schemaFile1 << "\" database file.\n";

        continue;
      }
    }
  }
}


static void
methodsTest(bool printMethodDifferences, OksClass *c1, OksClass *c2)
{
  const std::list<OksMethod *> * mlist = c1->direct_methods();
  OksMethod *m1, *m2;
	
  if(mlist) {
    for(std::list<OksMethod *>::const_iterator mi = mlist->begin(); mi != mlist->end(); ++mi) {
      OksMethod *m1 = *mi;
      const std::string & methodName = m1->get_name();

      if( !(m2 = c2->find_method(methodName)) ) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " There is no METHOD \"" << methodName << "\" defined in the \"" << schemaFile2 << "\" database file.\n";
	 		
        continue;
      }

      if(!(*m1 == *m2)) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " The METHOD \"" << methodName << "\" differed";

        if(!printMethodDifferences)
          std::cout << ".\n";
        else {
          int localAttrCount	= 0;
          const char * szTheMethod	= " The Method ";

          std::cout << ":\n";

          if(m1->get_description() != m2->get_description())
            std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                      << szTheMethod << "Description differed: \n"
                      << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << m1->get_description() << "\"\n"
                      << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << m2->get_description() << "\"\n";


          const std::list<OksMethodImplementation *> * m1i = m1->implementations();
          const std::list<OksMethodImplementation *> * m2i = m2->implementations();

          if(m1i || m2i) {
	    if(m2i == 0) {
              std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                        << szTheMethod << "Implementations differed: \n"
                        << "      in the FILE \"" << schemaFile1 << "\" it has implementations\n"
                        << "      in the FILE \"" << schemaFile2 << "\" it has no implementations\n";
	    }
	    else if(m1i == 0) {
              std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                        << szTheMethod << "Implementations differed: \n"
                        << "      in the FILE \"" << schemaFile1 << "\" it has no implementations\n"
                        << "      in the FILE \"" << schemaFile2 << "\" it has implementations\n";
	    }
	    else if(m1i->size() != m2i->size()) {
              std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                        << szTheMethod << "Implementations differed: \n"
                        << "      in the FILE \"" << schemaFile1 << "\" it has " << m1i->size() << " implementations\n"
                        << "      in the FILE \"" << schemaFile2 << "\" it has " << m2i->size() << " implementations\n";
	    }
	    else {
              std::list<OksMethodImplementation *>::const_iterator m1it = m1i->begin();
              std::list<OksMethodImplementation *>::const_iterator m2it = m2i->begin();
	      unsigned int icount = 0;
	      for(;m1it != m1i->end(); ++m1it, ++m2it) {
	        icount++;
	        OksMethodImplementation * i1 = *m1it;
	        OksMethodImplementation * i2 = *m2it;

		if(i1->get_language() != i2->get_language()) {
                  std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                            << szTheMethod << "Implementations " << icount << " differed: \n"
                            << "      in the FILE \"" << schemaFile1 << "\" it's language is \"" << i1->get_language() << "\"\n"
                            << "      in the FILE \"" << schemaFile2 << "\" it's language is \"" << i2->get_language() << "\"\n";
		
		}
		
		if(i1->get_prototype() != i2->get_prototype()) {
                  std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                            << szTheMethod << "Implementations " << icount << " differed: \n"
                            << "      in the FILE \"" << schemaFile1 << "\" it's prototype is \"" << i1->get_prototype() << "\"\n"
                            << "      in the FILE \"" << schemaFile2 << "\" it's prototype is \"" << i2->get_prototype() << "\"\n";
		
		}

		if(i1->get_body() != i2->get_body()) {
                  std::cout << "    " << classDiffsCount << '.' << localDiffsCount << '.' << ++localAttrCount
                            << szTheMethod << "Implementations " << icount << " differed: \n"
                            << "      in the FILE \"" << schemaFile1 << "\" it's body is \"" << i1->get_body() << "\"\n"
                            << "      in the FILE \"" << schemaFile2 << "\" it's body is \"" << i2->get_body() << "\"\n";
		
		}
	      }
	    }
	  }
        }
      }
    }
  }


  mlist = c2->direct_methods();
	
  if(mlist) {
    for(std::list<OksMethod *>::const_iterator mi = mlist->begin(); mi != mlist->end(); ++mi) {
      OksMethod *m2 = *mi;
      const std::string & methodName = m2->get_name();
      if( !(m1 = c1->find_method(methodName)) ) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " There is no METHOD \"" << methodName << "\" defined in the \"" << schemaFile1 << "\" database file.\n";

        continue;
      }
    }
  }
}


int main(int argc, char **argv)
{
  bool printAttributeDifferences = false;
  bool printRelationshipDifferences = false;
  bool printMethodDifferences = false;
  bool printSubClasses = false;

  if(argc == 1) {
    printUsage(std::cerr);
    return __BadCommandLine__;
  }

  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-a"))
      printAttributeDifferences = true;
    else if(!strcmp(argv[i], "-r"))
      printRelationshipDifferences = true;
    else if(!strcmp(argv[i], "-m"))
      printMethodDifferences = true;
    else if(!strcmp(argv[i], "-sb"))
      printSubClasses = true;
    else if(!strcmp(argv[i], "-all")) {
      printAttributeDifferences = true;
      printRelationshipDifferences = true;
      printMethodDifferences = true;
      printSubClasses = true;
    }
    else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      printUsage(std::cout);      
      return __Success__;
    }
    else if(!strcmp(argv[i], "-v")) {
      std::cout << "OKS kernel version " << OksKernel::GetVersion() << std::endl;      
      return __Success__;
    }
    else if(argc == (i + 2) && argv[i][0] != '-' && argv[i+1][0] != '-') {
      schemaFile1 = argv[i++];
      schemaFile2 = argv[i++];
    }
    else {
      std::cerr << "Unknown parameter: \"" << argv[i] << "\"\n\n";
      
      printUsage(std::cerr);
      return __BadCommandLine__;
    }
  }


  OksKernel kernel1;
  OksKernel kernel2;

  try {
  
    const char *szNoClasses = " No classes were found in \""; 
    const char *szExiting = ", exiting ...\n"; 

    kernel1.load_file(schemaFile1);
    kernel2.load_file(schemaFile2);
  
    const OksClass::Map& classes1 = kernel1.classes();
    const OksClass::Map& classes2 = kernel2.classes();

    if(classes1.empty()) {
      std::cerr << "ERROR:" << szNoClasses << schemaFile1 << "\" database file" << szExiting;
      return __ThereAreNoClasses__;
    }

    if(classes2.empty()) {
      std::cerr << "ERROR:" << szNoClasses << schemaFile2 << "\" database file" << szExiting;
      return __ThereAreNoClasses__;
    }

    OksClass::Map::const_iterator class_iterator1 = classes1.begin();
    OksClass::Map::const_iterator class_iterator2 = classes2.begin();
  
    std::cout << std::endl;


    classDiffsCount = 0;

    OksClass *c1, *c2;
  
    for(;class_iterator1 != classes1.end();++class_iterator1) {
      c1 = class_iterator1->second;
      const std::string& className = c1->get_name();

      if(!(c2 = kernel2.find_class(className))) {
        std::cout << "\n DIFFERENCE " << ++classDiffsCount << ". There is no class \"" << className << "\" in the \"" << schemaFile2 << "\" database file.\n";

        continue;
      }

      std::cout << "TESTING IN CLASSES \"" << className << "\"... ";
      std::cout.flush();


        //
        // Operators '==' and '!=' have completly different implementation:
        // - operator== is optimised for performance and
        //   checks addresses of classes and their names only
        // - operator!= checks all class properties
        //

      if((*c1 != *c2)  == false) {
        std::cout << " no differences were found\n";
        continue;
      }
	 	
      std::cout << std::endl << " DIFFERENCE " << ++classDiffsCount << '.'
                << " The CLASSES \"" << className << "\" differed:\n";
	 	
      localDiffsCount = 0;
	 		 	
      attributesTest(printAttributeDifferences, c1, c2);
      relationshipsTest(printRelationshipDifferences, c1, c2);
      methodsTest(printMethodDifferences, c1, c2);

      if(c1->get_description() != c2->get_description()) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " The Class Descriptions differed: \n"
                  << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << c1->get_description() << "\"\n"
                  << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << c2->get_description() << "\"\n";
      }

      if(c1->get_is_abstract() != c2->get_is_abstract()) {
        std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                  << " The Class Abstraction differed: \n"
                  << "      in the FILE \"" << schemaFile1 << "\" it is: \"" << (c1->get_is_abstract() ? "abstract" : "not abstract") << "\"\n"
                  << "      in the FILE \"" << schemaFile2 << "\" it is: \"" << (c2->get_is_abstract() ? "abstract" : "not abstract") << "\"\n";
      }


      { /* DIRECT SUPERCLASSES TEST */

        const std::list<std::string *> * slist = c1->direct_super_classes();
        const std::string *spn;
	
	if(slist) {
	  for(std::list<std::string *>::const_iterator spi = slist->begin(); spi != slist->end(); ++spi) {
	    spn = *spi;
            if( !c2->find_super_class(*spn) ) {
              std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                        << " There is no SUPERCLASS \"" << *spn << "\" defined in the \"" << schemaFile2 << "\" database file.\n";
	 		
              continue;
            }
          }
        }

	slist = c2->direct_super_classes();

	if(slist) {
	  for(std::list<std::string *>::const_iterator spi = slist->begin(); spi != slist->end(); ++spi) {
	    spn = *spi;
            if( !c1->find_super_class(*spn) ) {
              std::cout << "   " << classDiffsCount << '.' << ++localDiffsCount
                        << " There is no SUPERCLASS \"" << *spn << "\" defined in the \"" << schemaFile1 << "\" database file.\n";

              continue;
            }
          }
        }
      } /* END OF DIRECT SUPERCLASSES TEST */


      if(printSubClasses) {
        std::cout << "   ALL SUBCLASS(ES) of the CLASS \"" << className << "\"\n";
        std::cout << "    in the FILE \"" << schemaFile1 << "\"\n"; ReportSubClasses(c1);
        std::cout << "    in the FILE \"" << schemaFile2 << "\"\n"; ReportSubClasses(c2);
      }
    }

    for(;class_iterator2 != classes2.end();++class_iterator2) {
      if(!kernel1.find_class(class_iterator2->second->get_name())) {
        std::cout << "\n DIFFERENCE " << ++classDiffsCount << ". There is no class \"" << class_iterator2->second->get_name() << "\" in the \"" << schemaFile1 << "\" database file.\n";
      }
    }

    std::cout << "\nFound " << classDiffsCount << " differences, exiting...\n";
  }

  catch (oks::exception & ex) {
    std::cerr << "Caught oks exception:\n" << ex << std::endl;
    return __CannotReadFile__;
  }

  catch (std::exception & e) {
    std::cerr << "Caught standard C++ exception: " << e.what() << std::endl;
    return __CannotReadFile__;
  }

  return (classDiffsCount > __StartOfBadStatus__ ? __StartOfBadStatus__ : classDiffsCount);
}
