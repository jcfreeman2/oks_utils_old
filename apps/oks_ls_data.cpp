#include <list>

#include "CoralBase/Exception.h"
#include "CoralKernel/Context.h"
#include <CoralBase/Attribute.h>
#include <CoralBase/AttributeList.h>
#include <CoralBase/AttributeSpecification.h>

#include "RelationalAccess/ITransaction.h"
#include "RelationalAccess/ConnectionService.h"
#include "RelationalAccess/ISessionProxy.h"
#include <RelationalAccess/ISchema.h>
#include <RelationalAccess/ITable.h>
#include <RelationalAccess/ITransaction.h>
#include <RelationalAccess/IQuery.h>
#include <RelationalAccess/ICursor.h>
#include <RelationalAccess/SchemaException.h>

#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>

#include <ers/ers.h>

#include <oks/ral.h>
#include <oks/tz.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // The struct describes table's row with 5 items, separator and fill chars.
  // It is used to print separator line, title row and application parameters

struct TableRow {
  char m_fill;
  char m_separator;
  std::string m_items[7];
  TableRow(const char fill, const char separator) : m_fill(fill), m_separator(separator) { ; }
  TableRow(const char fill, const char separator, int sv, int dv, int bv, const std::string& release,
           const std::string& date, const std::string& user, const std::string& host, const std::string& description,
	   long num_of_obj = 0, long num_of_attr = 0, long num_of_rel = 0) : m_fill(fill), m_separator(separator) {
    if(sv || dv || bv) {
      std::ostringstream s; s << sv << '.' << dv;
      if(bv != dv) s << '.' << bv;
      m_items[0] = s.str();
    }
    m_items[1] = release;
    m_items[2] = date;
    m_items[3] = user;
    m_items[4] = host;
    if(num_of_obj || num_of_attr || num_of_rel) {
      std::ostringstream s2;
      s2 << num_of_obj << ':' << num_of_attr << ':' << num_of_rel;
      m_items[5] = s2.str();
    }
    m_items[6] = description;
  }
  TableRow(const char fill, const char separator, const std::string& s1, const std::string& s2,
           const std::string& s3, const std::string& s4, const std::string& s5,
	   const std::string& s6, const std::string& s7) : m_fill(fill), m_separator(separator) {
    m_items[0] = s1;
    m_items[1] = s2;
    m_items[2] = s3;
    m_items[3] = s4;
    m_items[4] = s5;
    m_items[5] = s6;
    m_items[6] = s7;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
usage()
{
  std::cout <<
    "usage: oks_ls_data\n"
    "   -c | --connect-string connect_string\n"
    "   -w | --working-schema schema_name\n"
    "   [-l | --list-releases]\n"
    "   [-s | --schema-version schema_version]\n"
    "   [-b | --base-data-only]\n"
    "   [-z | --show-size]\n"
    "   [-u | --show-usage]\n"
    "   [-d | --show-description]\n"
    "   [-t | --sorted-by parameters]\n"
    "   [-r | --release release_name]\n"
    "   [-e | --user user_name_pattern]\n"
    "   [-o | --host hostname_pattern]\n"
    "   [-p | --partition partition_name_pattern]\n"
    "   [-S | --archived-since timestamp]\n"
    "   [-T | --archived-till timestamp]\n"
    "   [-Z | --time-zone name]\n"
    "   [-v | --verbose-level verbosity_level]\n"
    "   [-h | --help]\n"
    "\n"
    "Options/Arguments:\n"
    "   -c connect_string    database connection string\n"
    "   -w schema_name       name of working schema\n"
    "   -l                   list releases\n"
    "   -s schema_version    print out data of this particular version (0 = HEAD version)\n"
    "   -b                   print out base data versions only\n"
    "   -z                   print size of version (i.e. number of relational rows to store it)\n"
    "   -u                   show who, when, where and how used given version\n"
    "   -d                   show description\n"
    "   -t parameters        sort output by several columns; the parameters may contain the following\n"
    "                        items (where first symbol is for ascending and second for descending order):\n"
    "                          v | V - sort by versions (run-number for usage [-u option]);\n"
    "                          t | T - sort by time;\n"
    "                          u | U - sort by user names;\n"
    "                          h | H - sort by hostnames;\n"
    "                          p | P - sort by partition names (i.e. by descriptions);\n"
//    "                          s | S - sort by sizes of archive, requires -u option.\n" // disabled because of Oracle bug
    "   -r release_name      show configurations for given release name\n"
    "   -e user_name_pattern show configurations for user names satisfying pattern (see syntax description below)\n"
    "   -o hostname_pattern  show configurations for hostnames satisfying pattern (see syntax description below)\n"
    "   -p partition_pattern show configurations or usage for partition names satisfying pattern (see syntax description below)\n"
    "   -S since_timestamp   show configurations archived or used since given moment (see timestamp format description below)\n"
    "   -T till_timestamp    show configurations archived before given moment (see timestamp format description below)\n"
    "   -Z name              time zone name (run \"-Z list-regions\" to see all supported time zones)\n"
    "   -v verbosity_level   set verbose output level (0 - silent, 1 - normal, 2 - extended, 3 - debug, ...)\n"
    "   -h                   print this message\n"
    "\n"
    "Description:\n"
    "   The utility prints out details of oks data versions archived in relational database.\n"
    "   The Version is shown as sv.dv[.bv], where:\n"
    "    * sv = schema version;\n"
    "    * dv = data version;\n"
    "    * bv = base version (optional, only appears for incremental data versions).\n"
    "   The Size (numbers of relational rows) is reported in form x:y:z, where the:\n"
    "    * x = number of new[/deleted/updated] objects;\n"
    "    * y = number of new[/deleted/updated] attribute values;\n"
    "    * z = number of new[/deleted/updated] relationship values.\n"
    "   By default the input timestamps are expected and the results are printed in UTC.\n"
    "   To use specific local time run program with -Z option (e.g. \"-Z \'Europe/Zurich\'\" for CERN local time).\n"
    "   The timestamps to be provided in ISO 8601 format: \"YYYY-MM-DD HH:MM:SS\".\n"
    "   The allowed wildcard characters used to select by user, host and partition names are:\n"
    "    % (i.e. percent symbol)     - any string of zero or more characters;\n"
    "    _ (i.e. underscore symbol)  - any single character.\n\n";
//    "    [ ] - any single character within the specified range ([a.f]) or set ([abcdef]);\n"
//    "    [^] - any single character not within the specified range ([^a.d]) or set ([^abcd]).\n\n";
}

static void
no_param(const char * s)
{
  std::cerr << "ERROR: no parameter for " << s << " provided\n\n";
}

static bool
has_a_pattern(const std::string& s)
{
  return(
    ( s.find("%") != std::string::npos ) ||
    ( s.find("_") != std::string::npos ) ||
    ( s.find("[") != std::string::npos && s.find("]") != std::string::npos) ? true : false
  );
}

int main(int argc, char *argv[])
{

    // parse and check command line

  int verbose_level = 1;
  std::string connect_string;
  std::string working_schema;
  int schema_version = -1;  // -1 -> ALL, 0 -> HEAD, >0 -> exact version
  bool ls_base_versions_only = false;    // -b
  bool ls_get_size = false;              // -z
  bool ls_usage = false;                 // -u
  bool ls_description = false;           // -d
  bool list_db = false;                  // -l
  std::string release;
  std::string user;
  std::string host;
  std::string partition;
  std::string acrhived_since;
  std::string acrhived_till;
  std::string sorted_by;
  const char * tmp_acrhived_since(0);
  const char * tmp_acrhived_till(0);
  const char * tz(0);
  boost::local_time::time_zone_ptr tz_ptr;

  for(int i = 1; i < argc; i++) {
    const char * cp = argv[i];

    if(!strcmp(cp, "-h") || !strcmp(cp, "--help")) { usage(); return (EXIT_SUCCESS); }
    else if(!strcmp(cp, "-b") || !strcmp(cp, "--base-data-only")) { ls_base_versions_only = true; }
    else if(!strcmp(cp, "-l") || !strcmp(cp, "--list-releases")) { list_db = true; }
    else if(!strcmp(cp, "-z") || !strcmp(cp, "--show-size")) { ls_get_size = true; }
    else if(!strcmp(cp, "-u") || !strcmp(cp, "--show-usage")) { ls_usage = true; }
    else if(!strcmp(cp, "-d") || !strcmp(cp, "--show-description")) { ls_description = true; }

    else if(!strcmp(cp, "-t") || !strcmp(cp, "--sorted-by")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { sorted_by = argv[i]; }
    }
    else if(!strcmp(cp, "-r") || !strcmp(cp, "--release")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { release = argv[i]; }
    }
    else if(!strcmp(cp, "-e") || !strcmp(cp, "--user")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { user = argv[i]; }
    }
    else if(!strcmp(cp, "-o") || !strcmp(cp, "--host")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { host = argv[i]; }
    }
    else if(!strcmp(cp, "-p") || !strcmp(cp, "--partition")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { partition = argv[i]; }
    }
    else if(!strcmp(cp, "-v") || !strcmp(cp, "--verbose-level")) {
      if(++i == argc) { no_param(cp); return (EXIT_FAILURE); } else { verbose_level = atoi(argv[i]); }
    }
    else if(!strcmp(cp, "-s") || !strcmp(cp, "--schema-version")) {
      if(++i == argc) { no_param(cp); return (1); } else { schema_version = atoi(argv[i]);}
    }
    else if(!strcmp(cp, "-c") || !strcmp(cp, "--connect-string")) {
      if(++i == argc) { no_param(cp); return (1); } else { connect_string = argv[i]; }
    }
    else if(!strcmp(cp, "-w") || !strcmp(cp, "--working-schema")) {
      if(++i == argc) { no_param(cp); return (1); } else { working_schema = argv[i]; }
    }
    else if(!strcmp(cp, "-S") || !strcmp(cp, "--archived-since")) {
      if(++i == argc) { no_param(cp); return 1; } else { tmp_acrhived_since = argv[i]; }
    }
    else if(!strcmp(cp, "-T") || !strcmp(cp, "--archived-till")) {
      if(++i == argc) { no_param(cp); return 1; } else { tmp_acrhived_till = argv[i]; }
    }
    else if(!strcmp(cp, "-Z") || !strcmp(cp, "--time-zone")) {
      if(++i == argc) { no_param(cp); return 1; } else { tz = argv[i]; }
    }
    else {
      std::cerr << "ERROR: Unexpected parameter: \"" << cp << "\"\n\n";
      usage();
      return (EXIT_FAILURE);
    }
  }

  if (tz)
    {
      try
        {
          oks::tz::DB tz_db;

          if (!strcmp(tz, "list-regions"))
            {
              for (auto & i : tz_db.get_regions())
                {
                  std::cout << i << std::endl;
                }
              return 0;
            }

          tz_ptr = tz_db.get_tz_ptr(tz);
        }
      catch (const std::exception& ex)
        {
          std::cerr << "ERROR: time-zone db access failed: " << ex.what() << std::endl;
          return 2;
        }
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

  try
    {
      if (tmp_acrhived_since)
        {
          acrhived_since = oks::tz::posix_time_2_iso_string(
              oks::tz::str_2_posix_time(tmp_acrhived_since, tz_ptr, "archived-since")
          );
        }

      if (tmp_acrhived_till)
        {
          acrhived_till = oks::tz::posix_time_2_iso_string(
              oks::tz::str_2_posix_time(tmp_acrhived_till, tz_ptr, "archived-till")
          );
        }
    }
  catch (const std::exception& ex)
    {
      std::cerr << "ERROR: " << ex.what() << std::endl;
      return 2;
    }

  try {

    std::unique_ptr<coral::ConnectionService> connection;

    {
      std::unique_ptr<coral::ISessionProxy> session (oks::ral::start_coral_session(connect_string, coral::ReadOnly, connection, verbose_level));

      if(list_db) {
        coral::ITable& schema_table = session->schema(working_schema).tableHandle( "OKSSCHEMA" );
        std::unique_ptr< coral::IQuery > query( schema_table.newQuery() );
        query->setRowCacheSize( 10 );
	query->setDistinct();
        query->addToOutputList( "RELEASE" );
        coral::ICursor& cursor = query->execute();

        std::set<std::string> names;

        while(cursor.next()) {
          names.insert(cursor.currentRow().begin()->data<std::string>());
        }

        std::cout << "Found configuration archives for " << names.size() << " releases:\n";

        for(std::set<std::string>::const_iterator i = names.begin(); i != names.end(); ++i) {
          std::cout << " - \'" << *i << '\'' << std::endl;
        }
      }
      else {
        if(schema_version == 0) {
          schema_version = oks::ral::get_head_schema_version(session.get(), working_schema, release.c_str(), verbose_level, "  ");
          if(schema_version == 0) {
            throw std::runtime_error( "Cannot get HEAD schema version." );
          }
        }

        coral::ITable& archive_table = session->schema(working_schema).tableHandle( "OKSARCHIVE" );

        std::list<std::string> conditions_list;
        coral::AttributeList conditions;


        std::unique_ptr< coral::IQuery > query (session->schema(working_schema).newQuery());
        query->setRowCacheSize( 1000 );

        query->addToOutputList( "A.SCHEMAVERSION" );
        query->addToOutputList( "A.BASEVERSION" );
        query->addToOutputList( "A.VERSION" );
        query->addToOutputList( "B.RELEASE" );
        query->addToOutputList( "A.TIME" );
        query->addToOutputList( "A.CREATEDBY" );
        query->addToOutputList( "A.HOST" );

        if(ls_description) {
          query->addToOutputList( "A.DESCRIPTION" );
        }

        coral::IQueryDefinition& subQuery1 = query->defineSubQuery( "A" );
        subQuery1.addToTableList( "OKSDATA" );
        query->addToTableList( "A" );

        coral::IQueryDefinition& subQuery2 = query->defineSubQuery( "B" );
        subQuery2.addToTableList( "OKSSCHEMA" );
        query->addToTableList( "B" );

        conditions_list.push_back("A.SCHEMAVERSION = B.VERSION");

        /*if(ls_get_size) {
          struct {
            const char * var;
            const char * table;
          } ts[] = {
              {"X", "OKSOBJECT"  },
            {"Y", "OKSDATAVAL" },
            {"Z", "OKSDATAREL" }
          };

          for(unsigned int i = 0; i < 1 ; ++i) {
            std::string p = std::string(ts[i].var) + ".S";

            query->addToOutputList(p);
            query->addToTableList( ts[i].var );
            query->defineOutputType( p, "long");

            coral::IQueryDefinition& subQuery = query->defineSubQuery( ts[i].var );
            subQuery.addToOutputList( "count(*)", "S" );
            subQuery.addToOutputList( "SCHEMAVERSION" );
            subQuery.addToOutputList( "DATAVERSION");
            subQuery.addToTableList( ts[i].table );
            subQuery.setCondition( "SCHEMAVERSION > 0 group by SCHEMAVERSION, DATAVERSION" , coral::AttributeList());

            conditions_list.push_back(std::string("A.SCHEMAVERSION = ") + ts[i].var + ".SCHEMAVERSION");
            conditions_list.push_back(std::string("A.VERSION = ") + ts[i].var + ".DATAVERSION");
          }
        }*/

        if(schema_version != -1) {
          conditions.extend<int>( "ver" );
          conditions["ver"].data<int>() = schema_version;

          if(ls_base_versions_only) {
            conditions_list.push_back("A.SCHEMAVERSION = :ver AND A.VERSION = A.BASEVERSION");
          }
          else {
            conditions_list.push_back("A.SCHEMAVERSION = :ver");
          }
        }
        else if(ls_base_versions_only) {
          conditions_list.push_back("A.VERSION = A.BASEVERSION");
        }

        if(!release.empty()) {
          conditions.extend<std::string>( "rel" );
          conditions["rel"].data<std::string>() = release;
          conditions_list.push_back("B.RELEASE = :rel");
        }

        if(!partition.empty()) {
	  if(!ls_usage) {
            if(ls_description == false) {
              throw std::runtime_error( "cannot search by partition without \"-d\" or \"-u\" command line option" );
            }
            conditions.extend<std::string>( "prt1" );
            conditions["prt1"].data<std::string>() = std::string("%partition ") + partition + '%';
            conditions_list.push_back("A.DESCRIPTION like :prt1");
	  }
        }

        if(!acrhived_since.empty()) {
          conditions.extend<std::string>( "ars" );
          conditions["ars"].data<std::string>() = acrhived_since;
	  if(ls_usage) {
	    query->setDistinct();
            coral::IQueryDefinition& subQuery3 = query->defineSubQuery( "C" );
            subQuery3.addToTableList( "OKSARCHIVE" );
            query->addToTableList( "C" );
	    std::string condition("C.TIME >= :ars AND A.SCHEMAVERSION = C.SCHEMAVERSION AND A.VERSION = C.DATAVERSION");
	    if(!partition.empty()) {
              conditions.extend<std::string>( "prt2" );
              conditions["prt2"].data<std::string>() = partition;
	      condition += (has_a_pattern(partition) ? " AND C.PARTITION like :prt2" : " AND C.PARTITION = :prt2");
	    }
            conditions_list.push_back(condition);
	  }
	  else {
            conditions_list.push_back("A.TIME >= :ars");
	  }
        }

        if(!acrhived_till.empty()) {
          conditions.extend<std::string>( "art" );
          conditions["art"].data<std::string>() = acrhived_till;
          conditions_list.push_back("A.TIME <= :art");
        }

        if(!user.empty()) {
          conditions.extend<std::string>( "usr" );
          conditions["usr"].data<std::string>() = user;
          conditions_list.push_back(has_a_pattern(user) ? "A.CREATEDBY like :usr" : "A.CREATEDBY = :usr");
        }

        if(!host.empty()) {
          conditions.extend<std::string>( "hst" );
          conditions["hst"].data<std::string>() = host;
          conditions_list.push_back(has_a_pattern(host) ? "A.HOST like :hst" : "A.HOST = :hst");
        }

        if(!sorted_by.empty()) {
          for(const char * p = sorted_by.c_str(); *p != 0; ++p) {
            if(ls_description == false && (*p == 'p' || *p == 'P')) {
              throw std::runtime_error( "cannot sort by partition without \"-d\" command line option" );
            }

            switch (*p) {
              case 'v': query->addToOrderList("A.SCHEMAVERSION"); query->addToOrderList("A.VERSION"); break;
              case 'V': query->addToOrderList("A.SCHEMAVERSION DESC"); query->addToOrderList("A.VERSION DESC"); break;
              case 't': query->addToOrderList("A.TIME"); break;
              case 'T': query->addToOrderList("A.TIME DESC"); break;
              case 'u': query->addToOrderList("A.CREATEDBY"); break;
              case 'U': query->addToOrderList("A.CREATEDBY DESC"); break;
              case 'h': query->addToOrderList("A.HOST"); break;
              case 'H': query->addToOrderList("A.HOST DESC"); break;
              case 'p': query->addToOrderList("A.DESCRIPTION"); break;
              case 'P': query->addToOrderList("A.DESCRIPTION DESC"); break;
              //case 's': query->addToOrderList("X.S"); break;
              //case 'S': query->addToOrderList("X.S DESC"); break;
              default:
                std::stringstream text;
                text << "ERROR: wrong value \'" << *p << "\' in the command line parameter: \"--sorted-by\" or \"-t\"";
                throw std::runtime_error( text.str().c_str() );
            }
          }
        }


        std::string conditions_str;

        for(std::list<std::string>::const_iterator i = conditions_list.begin(); i != conditions_list.end(); ++i) {
          if(i != conditions_list.begin()) conditions_str += " AND ";
          conditions_str += *i;
        }

        query->setCondition( conditions_str, conditions );

        coral::ICursor& cursor = query->execute();


        std::vector<TableRow> rows;

          // fill the table

        rows.push_back(TableRow('=', '='));
        rows.push_back(TableRow(' ', '|', "Version","Release",(tz_ptr ? "Date (local time)" : "Date (UTC)"),"User","Host","Size","Description"));
        rows.push_back(TableRow('=', '='));

        struct {
          long num;
          coral::ITable& table;
        } nums[] = {
          {0, session->schema(working_schema).tableHandle( "OKSOBJECT" )},
          {0, session->schema(working_schema).tableHandle( "OKSDATAVAL" )},
          {0, session->schema(working_schema).tableHandle( "OKSDATAREL" )}
        };

        while(cursor.next()) {
          const coral::AttributeList& row = cursor.currentRow();
 
          std::string time, host, user;
          oks::ral::get_time_host_user(row, time, host, user, tz_ptr, "A.");
          std::string description; if(ls_description && !row["A.DESCRIPTION"].isNull()) description = row["A.DESCRIPTION"].data<std::string>();
          std::string r; if(!row["B.RELEASE"].isNull()) r = row["B.RELEASE"].data<std::string>();
          int sv = row["A.SCHEMAVERSION"].data<int>();
          int dv = row["A.VERSION"].data<int>();
          nums[0].num = nums[1].num = nums[2].num = 0;

          if(ls_get_size) {
            coral::AttributeList sizes_query_conditions;
            sizes_query_conditions.extend<int>( "sv" );
            sizes_query_conditions["sv"].data<int>() = sv;
            sizes_query_conditions.extend<int>( "dv" );
            sizes_query_conditions["dv"].data<int>() = dv;

            for(unsigned int i = 0; i < 3; ++i) {
              std::unique_ptr< coral::IQuery > query( nums[i].table.newQuery() );
              query->setRowCacheSize( 1 );
              query->addToOutputList( "COUNT(*)");
              query->defineOutputType( "COUNT(*)", "long");
              query->setCondition( "SCHEMAVERSION = :sv AND DATAVERSION = :dv", sizes_query_conditions );

              coral::ICursor& c = query->execute();

              if( c.next() ) {
                nums[i].num = (int)c.currentRow().begin()->data<long>();
              }
            }
          }

          rows.push_back(
            TableRow(' ', '|',
              sv, dv, row["A.BASEVERSION"].data<int>(),
              r, time, user, host, description,
              nums[0].num, nums[1].num, nums[2].num
            )
          );

          if(ls_usage) {
            std::unique_ptr< coral::IQuery > query( archive_table.newQuery() );
            coral::AttributeList archive_query_conditions;
            archive_query_conditions.extend<int>( "sv" );
            archive_query_conditions["sv"].data<int>() = sv;
            archive_query_conditions.extend<int>( "dv" );
            archive_query_conditions["dv"].data<int>() = dv;
            query->setRowCacheSize( 10 );
            query->addToOutputList( "TIME");
            query->addToOutputList( "PARTITION");
            query->addToOutputList( "CREATEDBY");
            query->addToOutputList( "HOST");
            query->addToOutputList( "RUNNUMBER");

            if(!sorted_by.empty()) {
              for(const char * p = sorted_by.c_str(); *p != 0; ++p) {

                switch (*p) {
                  case 'v': query->addToOrderList("RUNNUMBER"); break;
                  case 'V': query->addToOrderList("RUNNUMBER DESC"); break;
                  case 't': query->addToOrderList("TIME"); break;
                  case 'T': query->addToOrderList("TIME DESC"); break;
                  case 'u': query->addToOrderList("CREATEDBY"); break;
                  case 'U': query->addToOrderList("CREATEDBY DESC"); break;
                  case 'h': query->addToOrderList("HOST"); break;
                  case 'H': query->addToOrderList("HOST DESC"); break;
                  case 'p': query->addToOrderList("PARTITION"); break;
                  case 'P': query->addToOrderList("PARTITION DESC"); break;
                }
              }
            }

            std::string condition("SCHEMAVERSION = :sv AND DATAVERSION = :dv");

            if(!acrhived_since.empty()) {
              archive_query_conditions.extend<std::string>( "ars" );
              archive_query_conditions["ars"].data<std::string>() = acrhived_since;
	      condition += " AND TIME >= :ars";
	    }

            if(!acrhived_till.empty()) {
              archive_query_conditions.extend<std::string>( "art" );
              archive_query_conditions["art"].data<std::string>() = acrhived_till;
	      condition += " AND TIME <= :art";
	    }

	    if(!partition.empty()) {
              archive_query_conditions.extend<std::string>( "prt" );
              archive_query_conditions["prt"].data<std::string>() = partition;
	      condition += (has_a_pattern(partition) ? " AND PARTITION like :prt" : " AND PARTITION = :prt");
	    }

            query->setCondition( condition, archive_query_conditions );

            coral::ICursor& c = query->execute();

            while(c.next()) {
              const coral::AttributeList& ar_row = c.currentRow();
              std::string ar_time, ar_host, ar_user;
              oks::ral::get_time_host_user(ar_row, ar_time, ar_host, ar_user, tz_ptr);
              std::string p; if(!ar_row["PARTITION"].isNull()) p = ar_row["PARTITION"].data<std::string>();

              std::ostringstream s;
              s << "partition: " << p << " run: " << ar_row["RUNNUMBER"].data<int>();

              rows.push_back(TableRow(' ', '|', 0, 0, 0, r, ar_time, ar_user, ar_host, s.str()));
            }
          }
        }

        rows.push_back(TableRow('=', '='));

          // detect columns widths

        unsigned short cw[] = {1,1,1,1,1,1,1};
        {
          for(auto & i : rows) {
            for(unsigned short j = 0; j < sizeof(cw)/sizeof(unsigned short); ++j) {
              unsigned short len = i.m_items[j].size();
              if(len > cw[j]) cw[j]=len;
            }
          }
        }


          // print table

        {
          bool align_left[] = {false, true, true, true, true, false, true};

          for(std::vector<TableRow>::const_iterator i = rows.begin(); i != rows.end(); ++i) {
            std::cout.fill((*i).m_fill);
            for(unsigned short j = 0; j < sizeof(cw)/sizeof(unsigned short); ++j) {
              std::cout << (*i).m_separator << (*i).m_fill;
              std::cout.setf((align_left[j] ? std::ios::left : std::ios::right), std::ios::adjustfield);
              std::cout.width(cw[j]);
              std::cout << (*i).m_items[j] << (*i).m_fill;
            }
            std::cout << (*i).m_separator << std::endl;
          }
        }

      }

      session->transaction().commit();

      VerboseMsg2("Ending user session..."); // delete session by unique_ptr<>
    }

    VerboseMsg2("Disconnecting..."); // delete connection by unique_ptr<>
  }

  catch ( coral::Exception& e ) {
    std::cerr << "CORAL exception: " << e.what() << std::endl;
    return 1;
  }

  catch ( std::exception& e ) {
    std::cerr << "Standard C++ exception : " << e.what() << std::endl;
    return 2;
  }

  catch ( ... ) {
    std::cerr << "Exception caught (...)" << std::endl;
    return 3;
  }

  VerboseMsg2("Exiting...");

  return 0;
}
