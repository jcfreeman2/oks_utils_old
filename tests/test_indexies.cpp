#include <iostream>
#include <stdlib.h>

#include <oks/kernel.h>
#include <oks/object.h>
#include <oks/class.h>
#include <oks/index.h>
#include <oks/attribute.h>

const char *appName;
const char *appTitle = "OKS indices tester.";

static void printUsage(const char *appName)
{
  std::cerr << appTitle << " Oks Kernel version " << OksKernel::GetVersion() << "\n"
	  "Usage: " << appName << " [-h[elp]] [-v] SchemaFile DataFile ClassName AttributeName options [options]\n"
	  "  Options:\n"
	  "\te value              - search == \'value\'\n"
	  "\tl value              - search < \'value\'\n"
	  "\tle value             - search <= \'value\'\n"
	  "\tg value              - search > \'value\'\n"
	  "\tge value             - search >= \'value\'\n"
	  "\tlog value1 value2    - search < \'value1\' or > \'value2\'\n"
	  "\tloge value1 value2   - search < \'value1\' or >= \'value2\'\n"
	  "\tleog value1 value2   - search <= \'value1\' or > \'value2\'\n"
	  "\tleoge value1 value2  - search <= \'value1\' or >= \'value2\'\n"
	  "\teoe value1 value2    - search == \'value1\' or == \'value2\'\n"
	  "\teol value1 value2    - search == \'value1\' or < \'value2\'\n"
	  "\teole value1 value2   - search == \'value1\' or <= \'value2\'\n"
	  "\teog value1 value2    - search == \'value1\' or > \'value2\'\n"
	  "\teoge value1 value2   - search == \'value1\' or >= \'value2\'\n"
	  "\tlag value1 value2    - search < \'value1\' and > \'value2\'\n"
	  "\tlage value1 value2   - search < \'value1\' and >= \'value2\'\n"
	  "\tleag value1 value2   - search <= \'value1\' and > \'value2\'\n"
	  "\tleage value1 value2  - search <= \'value1\' and >= \'value2\'\n"
	  "\teae value1 value2    - search == \'value1\' and == \'value2\'\n"
	  "\teal value1 value2    - search == \'value1\' and < \'value2\'\n"
	  "\teale value1 value2   - search == \'value1\' and <= \'value2\'\n"
	  "\teag value1 value2    - search == \'value1\' and > \'value2\'\n"
	  "\teage value1 value2   - search == \'value1\' and >= \'value2\'\n"
	  "\t-v    prints version\n"
	  "\t-h\n"
	  "\t-help prints this text\n\n";
}


int main(int argc, char** argv)
{
  OksKernel kernel;
  
  appName = argv[0];
  
  argv++;
  argc--;
  
  if(argc > 0) {
	if(
		!strcmp(argv[0], "-h") ||
		!strcmp(argv[0], "-help")
	) {
		printUsage(appName);
		return 0;
	}
	
	if(!strcmp(argv[0], "-v")) {
		std::cout << appTitle << " Oks Kernel version " << OksKernel::GetVersion() << std::endl;
		argv++;
		argc--;
	}
	
  }
  
  if(argc < 6) {
	printUsage(appName);
	return -1;
  }
  
  char *schemaFile = argv[0];
  char *dataFile = argv[1];
  char *className = argv[2];
  char *attributeName = argv[3];
  
  if(kernel.load_schema(schemaFile) == 0) {
    std::cerr << "ERROR[1]: Can`t load oks database schema file \"" << schemaFile << "\", exiting ...\n";
    return 1;
  }
  
  
  if(kernel.load_data(dataFile) == 0) {
    std::cerr << "ERROR[2]: Can`t load database data file \"" << dataFile << "\", exiting ...\n";
    return 2;
  }
  
  OksClass *classPnt = kernel.find_class(className);

  if(!classPnt) {
    std::cerr << "ERROR[3]: Can`t find class \"" << className << "\", exiting ...\n";
    return 3;
  }

  OksAttribute *attributePnt = classPnt->find_attribute(attributeName);

  if(!attributePnt) {
    std::cerr << "ERROR[4]: Can`t find attribute \"" << attributeName << "\" in class \"" << className << "\", exiting ...\n";
    return 4;
  }

  OksIndex index(classPnt, attributePnt);

  argv += 4;
  argc -= 4;
  
  try { while(argc > 1) {
    std::cout << std::endl;

    OksObject::List * olist = 0;
    OksData d, d2;

    if(!strcmp(argv[0], "e")) {
      d.SetValues(argv[1], attributePnt);
      
      std::cout << "Find all equal to " << d << std::endl;
      olist=index.FindEqual(&d);
      argv += 2;
      argc -= 2;
    }
    else if(!strcmp(argv[0], "l")) {
      d.SetValues(argv[1], attributePnt);
      
      std::cout << "Find all less then " << d << std::endl;
      olist=index.FindLess(&d);
      argv += 2;
      argc -= 2;
    }
    else if(!strcmp(argv[0], "le")) {
      d.SetValues(argv[1], attributePnt);
      
      std::cout << "Find all less-equal then " << d << std::endl;
      olist=index.FindLessEqual(&d);
      argv += 2;
      argc -= 2;
    }
    else if(!strcmp(argv[0], "g")) {
      d.SetValues(argv[1], attributePnt);
      
      std::cout << "Find all greate then " << d << std::endl;
      olist=index.FindGreat(&d);
      argv += 2;
      argc -= 2;
    }
    else if(!strcmp(argv[0], "ge")) {
      d.SetValues(argv[1], attributePnt);
      
      std::cout << "Find all greate-equal then " << d << std::endl;
      olist=index.FindGreatEqual(&d);
      argv += 2;
      argc -= 2;
    }
    else if(!strcmp(argv[0], "log")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all less then " << d << " or greate then " << d2 << std::endl;
      olist=index.FindLessOrGreat(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "loge")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all less then " << d << " or greate-equal then " << d2 << std::endl;
      olist=index.FindLessOrGreatEqual(&d, &d2);
      argv += 3;
      argc -= 3;
      }
    else if(!strcmp(argv[0], "leog")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all less-equal then " << d << " or greate then " << d2 << std::endl;
      olist=index.FindLessEqualOrGreat(&d, &d2);
      argv += 3;
      argc -= 3;
      }
    else if(!strcmp(argv[0], "leoge")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all less-equal then " << d << " or greate-equal then " << d2 << std::endl;
      olist=index.FindLessEqualOrGreatEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eoe")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " or equal to " << d2 << std::endl;
      olist=index.FindEqualOrEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eol")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " or less then " << d2 << std::endl;
      olist=index.FindEqualOrLess(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eole")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " or less-equal then " << d2 << std::endl;
      olist=index.FindEqualOrLessEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eog")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " or greate then " << d2 << std::endl;
      olist=index.FindEqualOrGreat(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eoge")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " or greate-equal then " << d2 << std::endl;
      olist=index.FindEqualOrGreatEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "lag")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all less then " << d << " and greate then " << d2 << std::endl;
      olist=index.FindLessAndGreat(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "lage")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all less then " << d << " and greate-equal then " << d2 << std::endl;
      olist=index.FindLessAndGreatEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "leag")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all less-equal then " << d << " and greate then " << d2 << std::endl;
      olist=index.FindLessEqualAndGreat(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "leage")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all less-equal then " << d << " and greate-equal then " << d2 << std::endl;
      olist=index.FindLessEqualAndGreatEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eae")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " and equal to " << d2 << std::endl;
      olist=index.FindEqualAndEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eal")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " and less then " << d2 << std::endl;
      olist=index.FindEqualAndLess(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eale")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " and less-equal then " << d2 << std::endl;
      olist=index.FindEqualAndLessEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eag")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " and greate then " << d2 << std::endl;
      olist=index.FindEqualAndGreat(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else if(!strcmp(argv[0], "eage")) {
      d.SetValues(argv[1], attributePnt);
      d2.SetValues(argv[2], attributePnt);
      
      std::cout << "Find all equal to " << d << " and greate-equal then " << d2 << std::endl;
      olist=index.FindEqualAndGreatEqual(&d, &d2);
      argv += 3;
      argc -= 3;
    }
    else {
      std::cerr << "ERROR[5]: Unknown option \"" << argv[0] << "\", exiting ...\n";
      return 5;
    }
    
    if(olist) {
      std::cout << "Query returns " << olist->size() << " objects:\n";
    
      OksObject::List::iterator it = olist->begin();
    
      size_t idLength = 5, valueLength = 7;

      for(;it != olist->end();++it) {
        OksData *d((*it)->GetAttributeValue(attributeName));
        std::string s = d->str();

        if((*it)->GetId().length() > idLength) idLength = (*it)->GetId().length();
        if(s.length() > valueLength) valueLength = s.length();
      }

      std::cout.fill('=');
      std::cout.width(idLength + valueLength + 7);
      std::cout << "" << std::endl;
      
      std::cout.fill(' ');

      std::cout.width(idLength + 3);
      std::cout << '|' << "Id |";

      std::cout.width(valueLength + 3);
      std::cout << "Value |" << std::endl;

      std::cout.fill('=');
      std::cout.width(idLength + valueLength + 7);
      std::cout << "" << std::endl;

      std::cout.fill(' ');

      for(it = olist->begin();it != olist->end();++it) {
        OksData *d((*it)->GetAttributeValue(attributeName));

        std::cout.width(idLength);
        std::cout << '|' << ' ' << (*it)->GetId().c_str() << ' ' << '|'; 

        std::cout.width(valueLength);

        std::string s = d->str();
        std::cout << ' ' << s << ' ' << '|' << std::endl;
      }

      std::cout.fill('=');
      std::cout.width(idLength + valueLength + 7);
      std::cout << "" << std::endl;

      delete olist;
    }
    else
      std::cout << "Nothing was found\n";

    std::cout << std::endl << std::endl;
  }
  } catch(oks::exception & ex) {
    std::cerr << "Caught exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

