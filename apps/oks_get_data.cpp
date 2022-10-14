#include <unistd.h>
#include <iostream>
#include <stdexcept>

#include "CoralBase/Exception.h"
#include "CoralKernel/Context.h"
#include "RelationalAccess/ITransaction.h"
#include "RelationalAccess/ConnectionService.h"
#include "RelationalAccess/ISessionProxy.h"

#include <oks/kernel.h>
#include <oks/ral.h>

static void
usage()
{
  std::cout <<
    "usage: oks_get_data\n"
    "       -c | --connect-string connect_string\n"
    "       -w | --working-schema schema_name\n"
    "       [-t | --tag data_tag]\n"
    "       [-e | --head-data-version [release-name]]\n"
    "       [-s | --schema-version schema_version]\n"
    "       [-n | --data-version data_version]\n"
    "       [-m | --out-schema-file schema_file]\n"
    "       [-f | --out-data-file data_file]\n"
    "       [-q | --query class oks-query [-r | --reference-level ref-level]]\n"
    "       [-v | --verbose-level verbosity_level]\n"
    "       [-h | --help]\n"
    "\n"
    "Options/Arguments:\n"
    "       -c connect_string    database connection string\n"
    "       -w schema_name       name of working schema\n"
    "       -t data_tag          get data by tag\n"
    "       -e [release-name]    get HEAD data version (cannot be used with -n);\n"
    "                            if there is no -s, then for schema HEAD version of \'" TDAQ_CMT_RELEASE "\' or the explicit release\n"
    "       -s schema_version    get data for given schema version (extra -n or -e to get HEAD data version is required)\n"
    "       -n data_version      get data by version (extra -s is required, but cannot be used with -e)\n"
    "       -m out_schema_file   name for output schema file\n"
    "       -f out_data_file     name for output data file\n"
    "       -q class query       only get objects of given class satisfying query\n"
    "       -r ref-level         also get objects referencing above objects with given recursion level\n"
    "       -v verbosity_level   set verbose output level (0 - silent, 1 - normal, 2 - extended, 3 - debug, ...)\n"
    "       -h                   print this message\n"
    "\n"
    "Description:\n"
    "       The utility creates oks files for given version of data stored in relational database.\n\n";
}

static void
no_param(const char * s)
{
  std::cerr << "ERROR: no parameter for " << s << " provided\n\n";
}


int main(int argc, char *argv[])
{

    // parse and check command line

  int verbose_level = 1;
  std::string connect_string;
  std::string release;
  std::string tag;
  std::string working_schema;
  std::string schema_file;
  std::string data_file;
  std::string query;
  std::string class_name;
  long ref_level = 0;
  int schema_version = -1;
  int data_version = -1;
  bool remove_schema_file = false;
  bool remove_data_file = false;

  for(int i = 1; i < argc; i++) {
    const char * cp = argv[i];

    if(!strcmp(cp, "-h") || !strcmp(cp, "--help")) {
      usage();
      return (EXIT_SUCCESS);
    }
    else if(!strcmp(cp, "-v") || !strcmp(cp, "--verbose-level")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { verbose_level = atoi(argv[i]); }
    }
    else if(!strcmp(cp, "-e") || !strcmp(cp, "--head-data-version")) {
      if(data_version > 0) {
        std::cerr << "ERROR: use either explicit data version (parameter -n) or HEAD version for provided release name (parameter -e ...)\n";
        usage();
        return EXIT_FAILURE;
      }
      data_version = 0;
      if(argc > (i+1) && argv[i+1][0] != '-') {++i; release = argv[i]; }
    }
    else if(!strcmp(cp, "-t") || !strcmp(cp, "--tag")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { tag = argv[i];}
    }
    else if(!strcmp(cp, "-s") || !strcmp(cp, "--schema-version")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { schema_version = atoi(argv[i]); }
    }
    else if(!strcmp(cp, "-n") || !strcmp(cp, "--data-version")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else {
        if(!release.empty()) {
          std::cerr << "ERROR: use either explicit data version (parameter -n) or HEAD version for provided release name (parameter -e ...)\n";
          usage();
          return EXIT_FAILURE;
        }
        data_version = atoi(argv[i]);
      }
    }
    else if(!strcmp(cp, "-c") || !strcmp(cp, "--connect-string")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { connect_string = argv[i]; }
    }
    else if(!strcmp(cp, "-w") || !strcmp(cp, "--working-schema")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { working_schema = argv[i]; }
    }
    else if(!strcmp(cp, "-m") || !strcmp(cp, "--out-schema-file")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { schema_file = argv[i]; }
    }
    else if(!strcmp(cp, "-f") || !strcmp(cp, "--out-data-file")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { data_file = argv[i]; }
    }
    else if(!strcmp(cp, "-q") || !strcmp(cp, "--query")) {
      if(i >= argc-2) { no_param(cp); return (EXIT_FAILURE); } else { class_name = argv[++i]; query = argv[++i];}
    }
    else if(!strcmp(cp, "-r") || !strcmp(cp, "--reference-level")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { ref_level = atoi(argv[i]); }
    }
    else {
      std::cerr << "ERROR: Unexpected parameter: \"" << cp << "\"\n\n";
      usage();
      return (EXIT_FAILURE);
    }
  }

  if(data_version == -1 && tag.empty()) {
    std::cerr << "ERROR: the data version or tag is required\n";
    usage();
    return EXIT_FAILURE;
  }
  else if(data_version > 0 && schema_version <= 0) {
    std::cerr << "ERROR: an explicit data version cannot be used without schema version\n";
    usage();
    return EXIT_FAILURE;
  }
  else if(data_version != -1 && !tag.empty()) {
    std::cerr << "ERROR: both schema version and tag are defined; only one can be used\n";
    usage();
    return EXIT_FAILURE;
  }

  if(connect_string.empty()) {
    std::cerr << "ERROR: the connect string is required\n";
    usage();
    return EXIT_FAILURE;
  }

  if(working_schema.empty()) {
    std::cerr << "ERROR: the working schema is required\n";
    usage();
    return EXIT_FAILURE;
  }


    // create oks kernel

  VerboseMsg2("Creating OKS kernel...");
  ::OksKernel kernel;


  try {

      // create oks schema file

    if(schema_file.empty()) {
      schema_file = kernel.get_tmp_file("/tmp/unknown.schema.xml");
      VerboseMsg2("The name of output schema file was not provided.\n * use file \'" << schema_file << '\'');
      remove_schema_file = true;
    }

    VerboseMsg3("Creating OKS schema file...");
    ::OksFile * fh1 = kernel.new_schema(schema_file);


      // create oks data file

    if(data_file.empty()) {
      data_file = kernel.get_tmp_file("/tmp/unknown.data.xml");
      VerboseMsg2("The name of output data file was not provided.\n * use file \'" << data_file << '\'');
      remove_data_file = true;
    }

    VerboseMsg3("Creating OKS data file...");
    ::OksFile * fh2 = kernel.new_data(data_file);

    {
      std::unique_ptr<coral::ConnectionService> connection;
      {
        std::unique_ptr<coral::ISessionProxy> session (oks::ral::start_coral_session(connect_string, coral::ReadOnly, connection, verbose_level));

        if(!tag.empty() || data_version <= 0 || schema_version < 0) {
          oks::ral::get_data_version(session.get(), working_schema, tag, schema_version, data_version, release.c_str(), verbose_level);
          if(data_version <= 0 || schema_version < 0) {
            throw std::runtime_error( (data_version == 0) ? "Cannot get head data" : "Cannot get given data" );
          }
        }

        oks::ral::get_data(kernel, session.get(), working_schema, schema_version, data_version, verbose_level);

        VerboseMsg2("Committing...");
        session->transaction().commit();

        VerboseMsg2("Ending user session...");  // delete session by unique_ptr<>
      }

      VerboseMsg2("Disconnecting..."); // delete connection by unique_ptr<>
    }

    if(!query.empty()) {

      // compute data, if there is an explicit query

      OksObject::FSet objects;
      
      if(OksClass * c = kernel.find_class(class_name)) {
        OksQuery q(c, query);
      
        if(q.good()) {
          OksObject::List * l = c->execute_query(&q);
	  if(l && !l->empty()) {
	    for(OksObject::List::iterator i = l->begin(); i != l->end(); ++i) {
	      objects.insert(*i);
	      if(ref_level > 0) {
	        (*i)->references(objects, static_cast<unsigned long>(ref_level));
	      }
	    }
	  }
	  else {
            std::cerr << "ERROR: failed to find any object of class \"" << class_name << "\" satisfying query \"" << query << "\", exiting...\n";
            unlink(schema_file.c_str());
            unlink(data_file.c_str());
            return (EXIT_FAILURE);
	  }
        }
        else {
          std::cerr << "ERROR: failed to parse query \"" << query << "\" for class class \"" << class_name << "\", exiting...\n";
          unlink(schema_file.c_str());
          unlink(data_file.c_str());
          return (EXIT_FAILURE);
        }
      }
      else {
        std::cerr << "ERROR: cannot find class \"" << class_name << "\", exiting...\n";
        unlink(schema_file.c_str());
        unlink(data_file.c_str());
        return (EXIT_FAILURE);
      }

      if(verbose_level > 2) {
        std::cout << objects.size() << " objects to be saved as defined by query and reference-level parameters:\n";
	
	for(OksObject::FSet::const_iterator i = objects.begin(); i != objects.end(); ++i) {
	  std::cout << "  " << *i << std::endl;
	}
      }
      
      std::string trash_data_file = kernel.get_tmp_file("/tmp/trash.data.xml");
      OksFile * fh3 = kernel.new_data(trash_data_file);
      
      if(!fh3) {
        std::cerr << "ERROR: failed to create oks data file \'" << trash_data_file << "\'\n";
        unlink(schema_file.c_str());
        unlink(data_file.c_str());
        return (EXIT_FAILURE);
      }

      VerboseMsg3("Filter out objects not satisfying query...");
      for(OksObject::Set::const_iterator i = kernel.objects().begin(); i != kernel.objects().end(); ++i) {
        if(objects.find(*i) == objects.end()) {
	  (*i)->set_file(fh3);
	  VerboseMsg3(" - filter out object " << *i);
	}
      }

      unlink(trash_data_file.c_str());
    }

    VerboseMsg5("Dump data:\n" << kernel);

    if(remove_schema_file) {
      VerboseMsg2("Removing temporal schema file \'" << schema_file << "\'...");
      unlink(schema_file.c_str());
    }
    else {
      VerboseMsg2("Saving oks schema...");
      kernel.save_schema(fh1);
      fh2->add_include_file(schema_file);
    }

    if(remove_data_file) {
      VerboseMsg2("Removing temporal data file \'" << data_file << "\'...");
      unlink(data_file.c_str());
    }
    else {
      VerboseMsg2("Saving oks data...");
      kernel.save_data(fh2, true);
    }

    VerboseMsg2("Exiting...");
  }

  catch ( coral::Exception& e ) {
    std::cerr << "CORAL exception: " << e.what() << std::endl;
    return 1;
  }

  catch ( oks::exception & ex) {
    std::cerr << "OKS exception:\n" << ex << std::endl;
    return 2;
  }

  catch ( std::exception& e ) {
    std::cerr << "Standard C++ exception : " << e.what() << std::endl;
    return 3;
  }

  catch ( ... ) {
    std::cerr << "Exception caught (...)" << std::endl;
    return 4;
  }

  return 0;
}
