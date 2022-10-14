#include "CoralBase/Exception.h"
#include "CoralKernel/Context.h"
#include "RelationalAccess/ITransaction.h"
#include "RelationalAccess/ConnectionService.h"
#include "RelationalAccess/ISessionProxy.h"

#include <unistd.h>
#include <iostream>
#include <stdexcept>

#include <oks/kernel.h>
#include <oks/ral.h>

static void
usage()
{
  std::cout <<
    "usage: oks_get_schema\n"
    "       -c | --connect-string connect_string\n"
    "       -w | --working-schema schema_name\n"
    "       [-e | --head-schema-version [release-name]]\n"
    "       [-n | --schema-version schema_version]\n"
    "       [-f | --out-file schema_file]\n"
    "       [-v | --verbose-level verbosity_level]\n"
    "       [-h | --help]\n"
    "\n"
    "Options/Arguments:\n"
    "       -c connect_string    database connection string\n"
    "       -w schema_name       name of working schema\n"
    "       -e [release-name]    get head schema version for \'" TDAQ_CMT_RELEASE "\' release (or defined explicitly)\n"
    "       -n schema_version    get schema by version\n"
    "       -f out_schema_file   name for output schema file\n"
    "       -v verbosity_level   set verbose output level (0 - silent, 1 - normal, 2 - extended, 3 - debug, ...)\n"
    "       -h                   print this message\n"
    "\n"
    "Description:\n"
    "       The utility creates oks file for given version of schema stored in relational database.\n\n";
}


static void
no_param(const char * s)
{
  std::cerr << "ERROR: no parameter for " << s << " provided\n\n";
}


int main(int argc, char *argv[])
{

    // parse command line

  int verbose_level = 1;
  std::string connect_string;
  std::string working_schema;
  std::string password;
  std::string file;
  std::string release;
  int schema_version = -1;
  bool remove_file = false;

  for(int i = 1; i < argc; i++) {
    const char * cp = argv[i];

    if(!strcmp(cp, "-h") || !strcmp(cp, "--help")) {
      usage();
      return (EXIT_SUCCESS);
    }
    else if(!strcmp(cp, "-v") || !strcmp(cp, "--verbose-level")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { verbose_level = atoi(argv[i]); }
    }
    else if(!strcmp(cp, "-n") || !strcmp(cp, "--schema-version")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { schema_version = atoi(argv[i]); }
    }
    else if(!strcmp(cp, "-e") || !strcmp(cp, "--head-schema-version")) {
      schema_version = 0;
      if(argc > (i+1) && argv[i+1][0] != '-') {++i; release = argv[i]; }
    }
    else if(!strcmp(cp, "-c") || !strcmp(cp, "--connect-string")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { connect_string = argv[i]; }
    }
    else if(!strcmp(cp, "-w") || !strcmp(cp, "--working-schema")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { working_schema = argv[i]; }
    }
    else if(!strcmp(cp, "-f") || !strcmp(cp, "--out-file")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { file = argv[i]; }
    }
    else {
      std::cerr << "ERROR: Unexpected parameter: \"" << cp << "\"\n\n";
      usage();
      return (EXIT_FAILURE);
    }
  }

  if(schema_version == -1) {
    std::cerr << "ERROR: specify schema version or use -e parameter to get HEAD schema version\n";
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


    // create oks kernel and schema file

  VerboseMsg2("Creating OKS kernel...");

  ::OksKernel kernel;


  try {


    if(file.empty()) {
      file = kernel.get_tmp_file("/tmp/unknown.schema.xml");
      VerboseMsg2("The name of output schema file was not provided.\n * use file \'" << file << '\'');
      remove_file = true;
    }

    VerboseMsg3("Creating OKS schema file...");
    ::OksFile * fh = kernel.new_schema(file);


    {
      std::unique_ptr<coral::ConnectionService> connection;
      {
        std::unique_ptr<coral::ISessionProxy> session (oks::ral::start_coral_session(connect_string, coral::ReadOnly, connection, verbose_level));

        if(schema_version == 0) {
          schema_version = oks::ral::get_head_schema_version(session.get(), working_schema, release.c_str(), verbose_level);
          if(schema_version < 0) {
            throw std::runtime_error( (schema_version == 0) ? "Cannot get head schema" : "Cannot get given schema" );
          }
        }

        oks::ral::get_schema(kernel, session.get(), working_schema, schema_version, true, verbose_level);

        VerboseMsg5("Dump schema:\n" << kernel);

        VerboseMsg2("Committing...");
        session->transaction().commit();

        VerboseMsg2("Ending user session..."); // delete session by unique_ptr<>
      }

      VerboseMsg2("Disconnecting..."); // delete connection by unique_ptr<>
    }

    if(remove_file) {
      VerboseMsg2("Removing temporal schema file \'" << file << "\'...");
      unlink(file.c_str());
    }
    else {
      VerboseMsg2("Saving oks schema...");
      kernel.save_schema(fh);
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
