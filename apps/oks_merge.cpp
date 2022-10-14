/**
 *  \file oks_merge.cpp
 *
 *  This file is part of the OKS package.
 *  https://gitlab.cern.ch/atlas-tdaq-software/oks
 *
 *  This file contains the implementation of the OKS application to merge several data files into a single data files.
 */


#include "oks/kernel.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ers/ers.hpp"

ERS_DECLARE_ISSUE(
  oks_merge,
  BadCommandLine,
  "Bad command line: \"" << what << '\"',
  ((const char*)what)
)

ERS_DECLARE_ISSUE(
  oks_merge,
  BadQuery,
  "Failed to parse query \"" << query << "\" in class \"" << class_name << '\"',
  ((const char*)query)
  ((const char*)class_name)
)

ERS_DECLARE_ISSUE(
  oks_merge,
  BadClass,
  "Cannot find class \"" << class_name << '\"',
  ((const char*)class_name)
)

  /** Failed to read OKS database file. */

ERS_DECLARE_ISSUE(
  oks_merge,
  CaughtException,
  "Caught " << what << " exception: \'" << text << '\'',
  ((const char *)what)
  ((const char *)text)
)

////////////////////////////////////////////////////////////////////////////////////////////////////

std::string appTitle;

void printUsage(const char *appName, std::ostream& s)
{
  s << appTitle << "\n"
       "Usage: " << appName << "\n"
       "    [--class name-of-class [--query query [--print-references recursion-depth [class-name*] [--]]]\n"
       "    [--version]\n"
       "    [--help]\n"
       "    [--out-data-file data_file] [--out-data-file schema_file] inputfiles*\n"
       "\n"
       "  Options:\n"
       "    -o | --out-data-file datafilename      the out filename to store oks objects from input files\n"
       "    -s | --out-schema-file schemafilename  the out filename to store oks classes from input files\n"
       "    -c | --class class_name                dump given class (all objects or matching some query)\n"
       "    -q | --query query                     print objects matching query (can only be used with class)\n"
       "    -r | --print-references N C1 C2 ... CX print objects referenced by found objects (can only be used with query), where:\n"
       "                                            * the parameter N defines recursion depth for referenced objects (> 0)\n"
       "                                            * the optional set of names {C1 .. CX} defines [sub-]classes for above objects\n"
       "    -v | --version                         print version\n"
       "    -h | --help                            print this text\n"
       "\n"
       "  Description:\n"
       "    Merges several oks files into single schema and data files.\n"
       "\n";
}

enum __OksMergeExitStatus__ {
  __Success__ = 0,
  __NoDataFilesLoaded__,
  __NoSchemaFilesLoaded__,
  __BadCommandLine__,
  __NoOutputSchemaFile__,
  __BadQuery__,
  __NoSuchClass__,
  __ExceptionCaught__
};


static void
no_param(const char * s)
{
  Oks::error_msg("oks_merge") << "no parameter(s) for command line argument \'" << s << "\' provided\n\n";
  exit(__BadCommandLine__);
}

int
main(int argc, char *argv[])
{
  appTitle = "OKS merge. OKS kernel version ";
  appTitle += OksKernel::GetVersion();

  const char * appName = "oks_merge";

  std::string out_data_file; 
  std::string out_schema_file;

  const char * class_name = nullptr;
  const char * query = nullptr;
  long recursion_depth = 0;
  std::vector<std::string> ref_classes;


  if(argc == 1) {
    printUsage(appName, std::cerr);
    return __BadCommandLine__;
  }

  OksKernel kernel;
  kernel.set_use_strict_repository_paths(false);

  try {

    for(int i = 1; i < argc; i++) {
      const char * cp = argv[i];

      if(!strcmp(cp, "-h") || !strcmp(cp, "--help")) {
        printUsage(appName, std::cout);      
        return __Success__;
      }
      else if(!strcmp(cp, "-v") || !strcmp(cp, "--version")) {
        std::cout << appTitle << std::endl;      
        return __Success__;
      }
      else if(!strcmp(cp, "-o") || !strcmp(cp, "--out-data-file")) {
        if(++i == argc) { no_param(cp); } else { out_data_file = argv[i]; }
      }
      else if(!strcmp(cp, "-s") || !strcmp(cp, "--out-schema-file")) {
        if(++i == argc) { no_param(cp); } else { out_schema_file = argv[i]; }
      }
      else if(!strcmp(cp, "-c") || !strcmp(cp, "--class")) {
        if(++i == argc) { no_param(cp); } else { class_name = argv[i]; }
      }
      else if(!strcmp(cp, "-q") || !strcmp(cp, "--query")) {
        if(++i == argc) { no_param(cp); } else { query = argv[i]; }
      }
      else if(!strcmp(cp, "-r") || !strcmp(cp, "--print-references")) {
        if(++i == argc) { no_param(cp); } else { recursion_depth = atol(argv[i]); }
          int j = 0;
          for(; j < argc - i - 1; ++j) {
            if(argv[i+1+j][0] != '-') { ref_classes.push_back(argv[i+1+j]); } else { break; }
          }
          i += j;
      }
      else if(strcmp(cp, "--")) {
        kernel.load_file(cp);
      }
    }

    if(!out_data_file.empty() && kernel.data_files().empty()) {
      ers::fatal(oks_merge::BadCommandLine(ERS_HERE,"There were no data files loaded"));
      return __NoDataFilesLoaded__;
    }

    if(!out_schema_file.empty() && kernel.schema_files().empty()) {
      ers::fatal(oks_merge::BadCommandLine(ERS_HERE,"There were no schema files loaded"));
      return __NoSchemaFilesLoaded__;
    }

    if(query && !class_name) {
      ers::fatal(oks_merge::BadCommandLine(ERS_HERE,"Query can only be executed when class name is provided (use -c option)"));
      return __BadCommandLine__;
    }


    OksFile * data_file_h = 0;
    OksFile * schema_file_h = 0;

    if(!out_data_file.empty()) {
      data_file_h = kernel.new_data(out_data_file);
      kernel.set_active_data(data_file_h);
    }

    if(!out_schema_file.empty()) {
      schema_file_h = kernel.new_schema(out_schema_file);
      kernel.set_active_schema(schema_file_h);
    }

    if(!data_file_h && !schema_file_h) {
      ers::fatal(oks_merge::BadCommandLine(ERS_HERE,"There is no out schema file name defined"));
      return __NoOutputSchemaFile__;
    }

    if(schema_file_h) {
      for(OksClass::Map::const_iterator j = kernel.classes().begin(); j != kernel.classes().end(); ++j) {
        j->second->set_file(schema_file_h, false);
      }

      kernel.save_schema(schema_file_h);
    }

    if(data_file_h) {
      if(schema_file_h) {
        data_file_h->add_include_file(out_schema_file);
      }

      if(class_name && *class_name) {
        if(OksClass * c = kernel.find_class(class_name)) {
          if(query && *query) {
            OksQuery * q = new OksQuery(c, query);
            if(q->good()) {
              OksObject::List * objs = c->execute_query(q);
              size_t num = (objs ? objs->size() : 0);
              std::cout << "Found " << num << " matching query \"" << query << "\" in class \"" << class_name << "\"";
              if(num) {
                OksObject::FSet refs;
                std::cout << ':' << std::endl;
                while(!objs->empty()) {
                  OksObject * o = objs->front();
                  objs->pop_front();
                  if(recursion_depth > 0) {
                    oks::ClassSet all_ref_classes;
                    kernel.get_all_classes(ref_classes, all_ref_classes);
                    o->references(refs, recursion_depth, true, &all_ref_classes);
                    std::cout << " - " << o << " references " << refs.size() << " objects" << std::endl;
                  }
                  else {
                    refs.insert(o);
                  }
                }
                delete objs;
                for(OksObject::FSet::iterator i = refs.begin(); i != refs.end(); ++i) {
                  const_cast<OksObject *>(*i)->set_file(data_file_h, false);
                }
              }
              else {
                std::cout << std::endl;
              }
            }
            else {
              ers::fatal(oks_merge::BadQuery(ERS_HERE, query, class_name));
              return __BadQuery__;
            }
            delete q;
          }
          else {
            std::cout << *c << std::endl;
          }
        }
        else {
          ers::fatal(oks_merge::BadClass(ERS_HERE, class_name));
          return __NoSuchClass__;
        }
      }
      else {
        for(OksObject::Set::const_iterator j = kernel.objects().begin(); j != kernel.objects().end(); ++j) {
          (*j)->set_file(data_file_h, false);
        }
      }

      kernel.save_data(data_file_h);
    }

  }

  catch (oks::exception & ex) {
    ers::fatal(oks_merge::CaughtException(ERS_HERE, "OKS", ex.what()));
    return __ExceptionCaught__;
  }
  
  catch (std::exception & ex) {
    ers::fatal(oks_merge::CaughtException(ERS_HERE, "Standard C++", ex.what()));
    return __ExceptionCaught__;
  }

  catch (...) {
    ers::fatal(oks_merge::CaughtException(ERS_HERE, "unknown", ""));
    return __ExceptionCaught__;
  }

  return __Success__;
}
