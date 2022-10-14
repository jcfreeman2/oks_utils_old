#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>


#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>

#include <CoralBase/Attribute.h>
#include <CoralBase/AttributeList.h>
#include <CoralBase/AttributeSpecification.h>
#include <CoralBase/Exception.h>
#include <CoralKernel/Context.h>

#include <RelationalAccess/ConnectionService.h>
#include <RelationalAccess/IConnectionServiceConfiguration.h>
#include <RelationalAccess/ISessionProxy.h>
#include <RelationalAccess/ISchema.h>
#include <RelationalAccess/ITable.h>
#include <RelationalAccess/ITransaction.h>
#include <RelationalAccess/ITableDescription.h>
#include <RelationalAccess/ITableDataEditor.h>
#include "RelationalAccess/IOperationWithQuery.h"
#include <RelationalAccess/IQuery.h>
#include <RelationalAccess/ICursor.h>
#include <RelationalAccess/IBulkOperation.h>
#include <RelationalAccess/SchemaException.h>
#include <RelationalAccess/IRelationalService.h>
#include <RelationalAccess/IRelationalDomain.h>

#include "oks/attribute.h"
#include "oks/relationship.h"
#include "oks/method.h"
#include "oks/kernel.h"
#include "oks/ral.h"


namespace oks { namespace ral {

const char * s_release = TDAQ_CMT_RELEASE;

static void
set_time_host_user(coral::AttributeList& data)
{
    // set current time
  boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
  data["TIME"].data<std::string>() = boost::posix_time::to_iso_string(now);

    // set user name
  data["CREATEDBY"].data<std::string>() = OksKernel::get_user_name();

    // set host name
  data["HOST"].data<std::string>() = OksKernel::get_host_name();
}

void
get_time_host_user(const coral::AttributeList& data, std::string& time, std::string& host, std::string& user, boost::local_time::time_zone_ptr tz_ptr, const char * prefix)
{
  std::string time_col_name, host_col_name, user_col_name;

  if(prefix && *prefix) {
    time_col_name = prefix;
    host_col_name = prefix;
    user_col_name = prefix;
  }

  time_col_name += "TIME";
  host_col_name += "HOST";
  user_col_name += "CREATEDBY";

    // get time (in UTC or time zone provided by user)

  boost::posix_time::ptime t = boost::posix_time::from_iso_string(data[time_col_name].data<std::string>());
  if(tz_ptr) {
    time = boost::local_time::local_date_time(t, tz_ptr).to_string();
  }
  else {
    time = boost::posix_time::to_simple_string(t);
  }

    // get host and user

  host = data[host_col_name].data<std::string>();
  user = data[user_col_name].data<std::string>();
}

coral::ISessionProxy *
start_coral_session(const std::string& connect_string, int mode, std::unique_ptr<coral::ConnectionService>& connection_service, int verbose_level)
{
  { // load services, if it was never done
    static bool inited(false);
    if(inited == false) {
      coral::Context& context = coral::Context::instance();

      VerboseMsg4("Loading CORAL/Services/XMLAuthenticationService...");
      context.loadComponent( "CORAL/Services/XMLAuthenticationService" );

      VerboseMsg4("Loading CORAL/Services/RelationalService...");
      context.loadComponent( "CORAL/Services/RelationalService" );

      inited = true;
    }
  }

  connection_service.reset(new coral::ConnectionService());

  { // change service configuration, if it was never done
    static bool inited(false);
    if(inited == false) {
      connection_service->configuration().disablePoolAutomaticCleanUp();
      connection_service->configuration().setConnectionTimeOut( 0 );

      inited = true;
    }
  }

  VerboseMsg3("Connecting and opening new session...");
  std::unique_ptr<coral::ISessionProxy> session( connection_service->connect( connect_string, static_cast<coral::AccessMode>(mode) ) );

  if( !session.get() ) {
    std::string msg("Could not connect to ");
    msg += connect_string;
    throw std::runtime_error( msg.c_str());
  }

  VerboseMsg3("Starting a new transaction...");
  session->transaction().start(mode == coral::ReadOnly);

  return session.release();
}


////////////////////////////////////////////////////////////////////////////////
///                          GET HEAD SCHEMA VERSION                         ///
////////////////////////////////////////////////////////////////////////////////

int
get_head_schema_version(
            coral::ISessionProxy * session,
            const std::string& schema,
            const char * release,
            int verbose_level,
	    const char * dx)
{
  std::unique_ptr< coral::IQuery > query( session->schema(schema).tableHandle("OKSSCHEMA").newQuery() );
  query->setRowCacheSize( 1 );

  if(!release || !*release) release = s_release;

  VerboseMsg1(dx << "Get head schema version (release " << release << ")...");

  query->addToOutputList( "MAX(VERSION)");
  query->defineOutputType( "MAX(VERSION)", "int");

  coral::AttributeList conditionData;
  conditionData.extend<std::string>( "r" );

  conditionData["r"].data<std::string>() = release;
  query->setCondition( "RELEASE = :r", conditionData );

  coral::ICursor& cursor = query->execute();

  if( cursor.next() && !cursor.currentRow().begin()->isNull() ) {
    int schema_version = cursor.currentRow().begin()->data<int>();
    VerboseMsg2(dx << " * the version is " << schema_version);
    return schema_version;
  }

  VerboseMsg1(dx << " * no head schema version was found...");

  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///                          GET NEW SCHEMA VERSION                          ///
////////////////////////////////////////////////////////////////////////////////

int
get_max_schema_version(
            coral::ISessionProxy * session,
            const std::string& schema,
            int verbose_level,
            const char * dx)
{
  std::unique_ptr< coral::IQuery > query( session->schema(schema).tableHandle("OKSSCHEMA").newQuery() );
  query->setRowCacheSize( 1 );

  VerboseMsg1(dx << "Get head schema version...");

  query->addToOutputList( "MAX(VERSION)");
  query->defineOutputType( "MAX(VERSION)", "int");

  coral::ICursor& cursor = query->execute();

  if( cursor.next() && !cursor.currentRow().begin()->isNull() ) {
    int schema_version = cursor.currentRow().begin()->data<int>();
    VerboseMsg2(dx << " * the version is " << schema_version);
    return schema_version;
  }

  VerboseMsg1(dx << " * no head schema version was found...");

  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///                          GET ALL SCHEMA VERSIONS                         ///
////////////////////////////////////////////////////////////////////////////////

std::vector<int>
get_schema_versions(
            coral::ISessionProxy * session,
            const std::string& schema,
            const char * release,
            int verbose_level,
	    const char * dx)
{
  if(!release || !*release) release = s_release;

  VerboseMsg1(dx << "Get all schema versions (release " << release << ")...");

  std::unique_ptr< coral::IQuery > query( session->schema(schema).tableHandle("OKSSCHEMA").newQuery() );
  query->setRowCacheSize( 10 );
  query->addToOutputList( "DISTINCT VERSION" );
  query->addToOrderList( "VERSION DESC" );

  coral::AttributeList conditionData;
  conditionData.extend<std::string>( "r" );
  conditionData["r"].data<std::string>() = release;
  query->setCondition( "RELEASE = :r", conditionData );

  coral::ICursor& cursor = query->execute();

  std::vector<int> result;

  while( cursor.next() ) {
    int v = cursor.currentRow().begin()->data<int>();
    VerboseMsg2(dx << "  * add version " << v);
    result.push_back(v);
  }

  if(result.empty()) {
    VerboseMsg2(dx << " * no schema versions were found...");
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////
///                              GET DATA VERSION                            ///
////////////////////////////////////////////////////////////////////////////////

void
get_data_version(
            coral::ISessionProxy * session,
            const std::string& schema,
            const std::string& tag,
            int& schema_version,
            int& data_version,
            const char * release,
            int verbose_level,
	    const char * dx)
{
  data_version = -1;


    // if tag is not empty, get schema and data versions from the OksTag table

  if(!tag.empty()) {
    coral::ITable& table = session->schema(schema).tableHandle( "OKSTAG" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );
    query->setRowCacheSize( 1 );

    coral::AttributeList conditionData;
    conditionData.extend<std::string>( "tag" );

    VerboseMsg1(dx << "Get schema and data versions for tag \'" << tag << "\'...");
    query->addToOutputList( "SCHEMAVERSION");
    query->addToOutputList( "DATAVERSION");
    conditionData["tag"].data<std::string>() = tag;
    query->setCondition( "NAME = :tag", conditionData );

    coral::ICursor& cursor = query->execute();

    if( cursor.next() ) {
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      if(iColumn != cursor.currentRow().end()) {
        schema_version = iColumn->data<int>();
	++iColumn;
        if(iColumn != cursor.currentRow().end()) {
	  data_version = iColumn->data<int>();
          VerboseMsg2(dx << " * schema version number is " << schema_version << "\n" << dx << " * data version number is " << data_version);
          return;
        }
      }
    }

    std::ostringstream text;
    text << "Cannot get schema and data versions for tag \'" << tag << '\'' ;
    throw std::runtime_error( text.str() );
  }
  else {

      // detect head schema version, if it is not set explicitly

    if(schema_version <= 0) {
      if(!release || !*release) release = s_release;
      schema_version = oks::ral::get_head_schema_version(session, schema, release, verbose_level, dx);
      if(schema_version <= 0) {
        std::ostringstream s;
        s << "Cannot get HEAD schema version for release " << release;
        throw std::runtime_error( s.str().c_str() );
      }
    }
    
      // get head data version for given schema version

    coral::ITable& table = session->schema(schema).tableHandle( "OKSDATA" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );
    query->setRowCacheSize( 1 );

    coral::AttributeList conditionData;
    conditionData.extend<int>( "ver" );
    query->addToOutputList( "MAX(VERSION)");
    query->defineOutputType( "MAX(VERSION)", "int");
    conditionData["ver"].data<int>() = schema_version;
    query->setCondition( "SCHEMAVERSION = :ver", conditionData );

    coral::ICursor& cursor = query->execute();

    if( cursor.next() ) {
      data_version = (cursor.currentRow().begin()->isNull() ? 0 : cursor.currentRow().begin()->data<int>());
      VerboseMsg2(dx << " * head data version is " << data_version << " (schema version " << schema_version << ')');
      return;
    }

    std::ostringstream text;
    text << "Cannot get HEAD data version for schema version " << schema_version << '.';
    throw std::runtime_error( text.str() );
  }
}


////////////////////////////////////////////////////////////////////////////////
///                                PUT SCHEMA                                ///
////////////////////////////////////////////////////////////////////////////////

static void
throw_schema_exception(coral::SchemaException& e, const char * table)
{
  std::string text = std::string(
    "Could not insert a new row into the ") + table + " table.\n"
    "Got CORAL schema exception: " + e.what();
  throw std::runtime_error( text.c_str() );
}

void
put_schema(const OksClass::Map& cs,
            coral::ISessionProxy * session,
            const std::string& schema,
            int schema_version,
	    const std::string& description,
	    int verbose_level)
{
  VerboseMsg1("Put schema version: \'" << schema_version << "\'...");

  if(schema_version == 0) {
    VerboseMsg2("  Detecting version...");

    schema_version = oks::ral::get_max_schema_version(session, schema, verbose_level, "  ");
    
    if(schema_version <= 0) {
      schema_version = 1;
      VerboseMsg2("   * use version 1 to put first schema");
    }
    else {
      schema_version++;
      VerboseMsg2("   * use incremented version " << schema_version);
    }
  }


  VerboseMsg2("  Importing schema...");


    // create schema

  if(schema_version > 0) {
    VerboseMsg3("    * creating row in OksSchema table...");

    coral::AttributeList data;
    coral::ITable& table = session->schema(schema).tableHandle( "OKSSCHEMA" );
    table.dataEditor().rowBuffer( data );

    data["VERSION"].data<int>() = schema_version;
    data["DESCRIPTION"].data<std::string>() = description;
    data["RELEASE"].data<std::string>() = s_release;
    set_time_host_user(data);

    try {
      table.dataEditor().insertRow( data );
    }
    catch ( coral::SchemaException& e ) {
      throw_schema_exception(e, "OksSchema");
    }
  }
  else {
    schema_version = -schema_version;
    VerboseMsg3("   * updating existing OksSchema " << schema_version << " ...");
  }


  int position;


    // create classes

  {
    coral::AttributeList data;
    coral::ITable& table = session->schema(schema).tableHandle( "OKSCLASS" );
    table.dataEditor().rowBuffer( data );

    for(OksClass::Map::const_iterator class_it = cs.begin(); class_it != cs.end(); ++class_it)
    {
      VerboseMsg3("   * insert class \'" << class_it->second->get_name() << '\'');

      data["VERSION"].data<int>() = schema_version;
      data["NAME"].data<std::string>() = class_it->second->get_name();
      data["DESCRIPTION"].data<std::string>() = class_it->second->get_description();
      data["ISABSTRACT"].data<bool>() = (class_it->second->get_is_abstract() ? 1 : 0 );

      try {
        table.dataEditor().insertRow( data );
      }
      catch ( coral::SchemaException& e ) {
        throw_schema_exception(e, "OksClass");
      }
    }
  }


    // create superclasses, attributes, relationships and methods

  {
    coral::ITable& tableOksSuperClass = session->schema(schema).tableHandle( "OKSSUPERCLASS" );
    coral::AttributeList dataOksSuperClass;
    tableOksSuperClass.dataEditor().rowBuffer( dataOksSuperClass );

    coral::ITable& tableOksAttribute = session->schema(schema).tableHandle( "OKSATTRIBUTE" );
    coral::AttributeList dataOksAttribute;
    tableOksAttribute.dataEditor().rowBuffer( dataOksAttribute );

    coral::ITable& tableOksRelationship = session->schema(schema).tableHandle( "OKSRELATIONSHIP" );
    coral::AttributeList dataOksRelationship;
    tableOksRelationship.dataEditor().rowBuffer( dataOksRelationship );

    coral::ITable& tableOksMethod = session->schema(schema).tableHandle( "OKSMETHOD" );
    coral::AttributeList dataOksMethod;
    tableOksMethod.dataEditor().rowBuffer( dataOksMethod );

    coral::ITable& tableOksMethodImpl = session->schema(schema).tableHandle( "OKSMETHODIMPLEMENTATION" );
    coral::AttributeList dataOksMethodImpl;
    tableOksMethodImpl.dataEditor().rowBuffer( dataOksMethodImpl );

    for(OksClass::Map::const_iterator class_it = cs.begin(); class_it != cs.end(); ++class_it)
    {
      const OksClass * oks_class = class_it->second;

      VerboseMsg3("   * process class \'" << oks_class->get_name() << '\'');


        // create superclasses

      if(const std::list<std::string *> * superclasses = oks_class->direct_super_classes())
      {
        position = 1;
        for(std::list<std::string *>::const_iterator sclass_it = superclasses->begin(); sclass_it != superclasses->end(); ++sclass_it)
	{
          VerboseMsg4("     - add superclass \'" << **sclass_it << '\'');

          dataOksSuperClass["VERSION"].data<int>() = schema_version;
          dataOksSuperClass["PARENTNAME"].data<std::string>() = oks_class->get_name();
          dataOksSuperClass["PARENTPOSITION"].data<int>() = position++ ;
          dataOksSuperClass["VALUE"].data<std::string>() = **sclass_it;

          try {
            tableOksSuperClass.dataEditor().insertRow( dataOksSuperClass );
          }
          catch ( coral::SchemaException& e ) {
            throw_schema_exception(e, "OksSuperClass");
          }
        }
      }


        // create attributes

      if(const std::list<OksAttribute *> * attributes = oks_class->all_attributes()) {
        position = 1;
        for(std::list<OksAttribute *>::const_iterator i = attributes->begin(); i != attributes->end(); ++i)
	{
          VerboseMsg4("     - add attribute \'" << (*i)->get_name() << '\'');

          dataOksAttribute["VERSION"].data<int>() = schema_version;
          dataOksAttribute["PARENTNAME"].data<std::string>() = oks_class->get_name();
          dataOksAttribute["PARENTPOSITION"].data<int>() = position++;
	  dataOksAttribute["ISDIRECT"].data<bool>() = (oks_class->find_direct_attribute((*i)->get_name()) ? 1 : 0);
          dataOksAttribute["NAME"].data<std::string>() = (*i)->get_name();
          dataOksAttribute["DESCRIPTION"].data<std::string>() = (*i)->get_description();
          dataOksAttribute["TYPE"].data<std::string>() = (*i)->get_type();
          dataOksAttribute["RANGE"].data<std::string>() = (*i)->get_range();
          dataOksAttribute["FORMAT"].data<std::string>() = OksAttribute::format2str((*i)->get_format());
          dataOksAttribute["ISMULTIVALUE"].data<bool>() = ((*i)->get_is_multi_values() ? 1 : 0 );
          dataOksAttribute["MULTIVALUEIMPL"].data<std::string>() = "list"; // FIXME (IS 2014-08-04): drop this column in future releases
          dataOksAttribute["INITVALUE"].data<std::string>() = (*i)->get_init_value();
          dataOksAttribute["ISNOTNULL"].data<bool>() = ((*i)->get_is_no_null() ? 1 : 0 );

          try {
            tableOksAttribute.dataEditor().insertRow( dataOksAttribute );
          }
          catch ( coral::SchemaException& e ) {
            throw_schema_exception(e, "OksAttribute");
          }
        }
      }


        // create relationships

      if(const std::list<OksRelationship *> * relationships = oks_class->all_relationships())
      {
        position = 1;
        for(std::list<OksRelationship *>::const_iterator i = relationships->begin(); i != relationships->end(); ++i)
	{
          VerboseMsg4("     - add relationship \'" << (*i)->get_name() << '\'');

          dataOksRelationship["VERSION"].data<int>() = schema_version;
          dataOksRelationship["PARENTNAME"].data<std::string>() = oks_class->get_name();
          dataOksRelationship["PARENTPOSITION"].data<int>() = position++;
          dataOksRelationship["NAME"].data<std::string>() = (*i)->get_name();
	  dataOksRelationship["ISDIRECT"].data<bool>() = (oks_class->find_direct_relationship((*i)->get_name()) ? 1 : 0);
          dataOksRelationship["DESCRIPTION"].data<std::string>() = (*i)->get_description();
          dataOksRelationship["CLASSTYPE"].data<std::string>() = (*i)->get_type();
          dataOksRelationship["LOWCC"].data<std::string>() = OksRelationship::card2str((*i)->get_low_cardinality_constraint());
          dataOksRelationship["HIGHCC"].data<std::string>() = OksRelationship::card2str((*i)->get_high_cardinality_constraint());
          dataOksRelationship["ISCOMPOSITE"].data<bool>() = ( (*i)->get_is_composite() ? 1 : 0 );
          dataOksRelationship["ISEXCLUSIVE"].data<bool>() = ( (*i)->get_is_exclusive() ? 1 : 0 );
          dataOksRelationship["ISDEPENDENT"].data<bool>() = ( (*i)->get_is_dependent() ? 1 : 0 );
          dataOksRelationship["MULTIVALUEIMPL"].data<std::string>() = "list"; // FIXME (IS 2014-08-04): drop this column in future releases

          try {
            tableOksRelationship.dataEditor().insertRow( dataOksRelationship );
          }
          catch ( coral::SchemaException& e ) {
            throw_schema_exception(e, "OksRelationship");
          }
        }
      }


        // create methods

      if(const std::list<OksMethod *> * methods = oks_class->direct_methods())
      {
        position = 1;
        for(std::list<OksMethod *>::const_iterator i = methods->begin(); i != methods->end(); ++i) {
          VerboseMsg4("     - add method \'" << (*i)->get_name() << '\'');

          dataOksMethod["VERSION"].data<int>() = schema_version;
          dataOksMethod["PARENTNAME"].data<std::string>() = oks_class->get_name();
          dataOksMethod["PARENTPOSITION"].data<int>() = position++;
	  dataOksMethod["ISDIRECT"].data<bool>() = (oks_class->find_direct_method((*i)->get_name()) ? 1 : 0);
          dataOksMethod["NAME"].data<std::string>() = (*i)->get_name();
          dataOksMethod["DESCRIPTION"].data<std::string>() = (*i)->get_description();

          try {
            tableOksMethod.dataEditor().insertRow(dataOksMethod  );
          }
          catch ( coral::SchemaException& e ) {
            throw_schema_exception(e, "OksMethod");
          }
	  
	  if(const std::list<OksMethodImplementation *> * implementations = (*i)->implementations())
	  {
	    int position2 = 0;
	    for(std::list<OksMethodImplementation *>::const_iterator j = implementations->begin(); j != implementations->end(); ++j) {
              VerboseMsg4("       - add method implementation on \'" << (*j)->get_language() << '\'');

              dataOksMethodImpl["VERSION"].data<int>() = schema_version;
              dataOksMethodImpl["PARENTCLASSNAME"].data<std::string>() = oks_class->get_name();
              dataOksMethodImpl["PARENTMETHODNAME"].data<std::string>() = (*i)->get_name();
              dataOksMethodImpl["PARENTPOSITION"].data<int>() = position2++;
              dataOksMethodImpl["LANGUAGE"].data<std::string>() = (*j)->get_language();
              dataOksMethodImpl["PROTOTYPE"].data<std::string>() = (*j)->get_prototype();
              dataOksMethodImpl["BODY"].data<std::string>() = (*j)->get_body();

              try {
                tableOksMethodImpl.dataEditor().insertRow( dataOksMethodImpl );
              }
              catch ( coral::SchemaException& e ) {
                throw_schema_exception(e, "OksMethodImplementation");
              }
	    }
	  }
        }
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
///                                 PUT DATA                                 ///
////////////////////////////////////////////////////////////////////////////////


void
fill_attr_value(coral::AttributeList& data,
                coral::IBulkOperation * inserter,
		const OksData& d,
		int pos)
{
  const std::string *   str_val_ptr = 0;
  const int64_t *       int_val_ptr = 0;
  const double *        num_val_ptr = 0;

  int64_t v;
  double v2;
  std::string v3;

  switch(d.type) {
    case OksData::string_type:    str_val_ptr = d.data.STRING;                                        break;
    case OksData::enum_type:      str_val_ptr = d.data.ENUMERATION;                                   break;
    case OksData::s32_int_type:   v = static_cast<int64_t>(d.data.S32_INT); int_val_ptr = &v;         break;
    case OksData::u32_int_type:   v = static_cast<int64_t>(d.data.U32_INT); int_val_ptr = &v;         break;
    case OksData::s16_int_type:   v = static_cast<int64_t>(d.data.S16_INT); int_val_ptr = &v;         break;
    case OksData::u16_int_type:   v = static_cast<int64_t>(d.data.U16_INT); int_val_ptr = &v;         break;
    case OksData::u8_int_type:    v = static_cast<int64_t>(d.data.U8_INT); int_val_ptr = &v;          break;
    case OksData::s8_int_type:    v = static_cast<int64_t>(d.data.S8_INT); int_val_ptr = &v;          break;
    case OksData::s64_int_type:   v = d.data.S64_INT; int_val_ptr = &v;                               break;
    case OksData::u64_int_type:   v = static_cast<int64_t>(d.data.U64_INT); int_val_ptr = &v;         break;
    case OksData::bool_type:      v = static_cast<int64_t>(d.data.BOOL); int_val_ptr = &v;            break;
    case OksData::float_type:     v2 = static_cast<double>(d.data.FLOAT); num_val_ptr = &v2;          break;
    case OksData::double_type:    num_val_ptr = &d.data.DOUBLE;                                       break;
    case OksData::date_type:      v3 = boost::gregorian::to_iso_string(d.date()); str_val_ptr = &v3;  break;
    case OksData::time_type:      v3 = boost::posix_time::to_iso_string(d.time()); str_val_ptr = &v3; break;
    case OksData::class_type:     str_val_ptr = &d.data.CLASS->get_name();                            break;

    default:
      std::ostringstream text;
      text << "Internal error: unexpected type of attribute\'" << data["NAME"].data<std::string>() << "\' with value " << d;
      throw std::runtime_error( text.str() );
  }

  if(str_val_ptr) { data["STRINGVALUE"].setNull(false); if(str_val_ptr->size() < 4000) data["STRINGVALUE"].data<std::string>() = *str_val_ptr;
                                                        else data["STRINGVALUE"].data<std::string>() = str_val_ptr->substr(0, 3999); }
  else            { data["STRINGVALUE"].setNull(true); }

  if(int_val_ptr) { data["INTEGERVALUE"].setNull(false); data["INTEGERVALUE"].data<int64_t>() = *int_val_ptr; }
  else            { data["INTEGERVALUE"].setNull(true); }

  if(num_val_ptr) { data["NUMBERVALUE"].setNull(false); data["NUMBERVALUE"].data<double>() = *num_val_ptr; }
  else            { data["NUMBERVALUE"].setNull(true); }

  data["PARENTPOSITION"].data<int>() = pos;

  try {
    inserter->processNextIteration();
  }
  catch (coral::SchemaException& e) {
    std::ostringstream text;

    text <<
      "Bulk insert of a new row into the OksDataVal table has failed.\n"
      "Got CORAL schema exception: " << e.what() << "\n"
      "  DEBUG:\n"
      "     SCHEMAVERSION: " << data["SCHEMAVERSION"].data<int>() << "\n"
      "       DATAVERSION: " << data["DATAVERSION"].data<int>() << "\n"
      "         CLASSNAME: " << data["CLASSNAME"].data<std::string>() << "\n"
      "          OBJECTID: " << data["OBJECTID"].data<std::string>() << "\n"
      "              NAME: " << data["NAME"].data<std::string>() << "\n"
      "    PARENTPOSITION: " << pos << "\n"
      "             VALUE: " << d;

    throw std::runtime_error( text.str() );
  }
}

void
create_rel_value(coral::AttributeList& data,
                 coral::IBulkOperation * inserter,
		 int object_version,
		 const OksData& d,
		 int pos,
		 OksKernel * k2,
		 int base_version)
{
  if(d.type != OksData::object_type) {
    static bool allow_dr = ::getenv("OKS_RAL_ALLOW_DANGLING_REFERENCES");
    std::ostringstream text;

    text <<
      "ERROR: object \'" << data["OBJECTID"].data<std::string>() << '@' << data["CLASSNAME"].data<std::string>() << "\' has dangling reference on object " << d;

    if(allow_dr) {
      std::cerr << text.str() << ", ignoring..." << std::endl;
      return;
    }
    else {
      throw std::runtime_error( text.str() );
    }
  }

  if(OksObject * value = d.data.OBJECT) {
    int dv = (k2 ? base_version : object_version);

    data["PARENTPOSITION"].data<int>() = pos;
    data["VALUECLASSNAME"].data<std::string>() = value->GetClass()->get_name();
    data["VALUEOBJECTID"].data<std::string>() = value->GetId();
    data["VALUEVERSION"].data<int>() = dv;

    try {
      inserter->processNextIteration();
    }
    catch (coral::SchemaException& e) {
      std::ostringstream text;

      text <<
        "Bulk insert of a new row into the OksDataRel table has failed.\n"
        "Got CORAL schema exception: " << e.what() << "\n"
        "  DEBUG:\n"
        "     SCHEMAVERSION: " << data["SCHEMAVERSION"].data<int>() << "\n"
        "       DATAVERSION: " << object_version << "\n"
        "         CLASSNAME: " << data["CLASSNAME"].data<std::string>() << "\n"
        "          OBJECTID: " << data["OBJECTID"].data<std::string>() << "\n"
        "              NAME: " << data["NAME"].data<std::string>() << "\n"
        "    PARENTPOSITION: " << pos << "\n"
        "    VALUECLASSNAME: " << value->GetClass()->get_name() << "\n"
        "     VALUEOBJECTID: " << value->GetId() << "\n"
        "      VALUEVERSION: " << dv;

      throw std::runtime_error( text.str() );
    }
  }
}

static bool
compare_2_tables(coral::ISessionProxy * session,
                 const std::string& schema,
                 const char * table1_name,
                 const char * table2_name,
                 const char ** columns,
		 const std::string& cond1,
		 const std::string& cond2,
		 int real_base_version,
		 int data_version,
		 int verbose_level)
{
  std::unique_ptr< coral::IQuery > query( session->schema(schema).newQuery() );

  query->addToTableList(table1_name);
  for(unsigned int i = 0; columns[i]; ++i) {
    query->addToOutputList(columns[i]);
  }
  
  query->setCondition( cond1, coral::AttributeList() );

  coral::IQueryDefinition& query2 = query->applySetOperation( coral::IQueryDefinition::Minus );
  query2.addToTableList(table2_name);

  for(unsigned int i = 0; columns[i]; ++i) {
    query2.addToOutputList(columns[i]);
  }

  query2.setCondition( cond2, coral::AttributeList() );

  coral::ICursor& cursor = query->execute();

  if(cursor.next()) {
    VerboseMsg3("  The base data version " << real_base_version << " and new data version " << data_version << " are different (after " << table1_name << " vs. " << table2_name << " test)")

    if(verbose_level > 3) {
      unsigned long idx = 0;
      do {
        std::cout << "  " << ++idx << ": "; cursor.currentRow().toOutputStream( std::cout ) << std::endl;
      } while(cursor.next());
    }

    return true;
  }

  return false;
}
/*
static void
deleteRows(
  coral::ITable& table,
  const char * table_name,
  const char * data_version_column_name,
  int schema_version,
  int data_version,
  long num_of_rows,
  int verbose_level)
{
  std::string deleteCondition = std::string("SchemaVersion = :sv AND ") + data_version_column_name + " = :dv";
  coral::AttributeList deleteData;
  deleteData.extend<int>( "sv" );
  deleteData.extend<int>( "dv" );
  deleteData["sv"].data<int>() = schema_version;
  deleteData["dv"].data<int>() = data_version;

  try {
    VerboseMsg3("  Deleting " << num_of_rows << " rows from table \"" << table_name << "\" data-version " << data_version << ", schema-version " << schema_version << "...")
    long ret = table.dataEditor().deleteRows( deleteCondition, deleteData );
    if(ret != num_of_rows) {
      std::ostringstream text;
      text << "Could not delete " << num_of_rows << " rows from table \"" << table_name << "\" data-version "
           << data_version << ", schema-version " << schema_version << "; " << ret << " rows were deleted";
      throw std::runtime_error( text.str().c_str() );
    }
  }
  catch ( coral::SchemaException& e ) {
    throw_schema_exception(e, "OksData");
  }
}*/

static void
copy_table(const char * src_table,
            const char * dest_table,
            const std::string& cond,
            coral::ISessionProxy * session,
            const std::string& schema,
	    int verbose_level)
{
  coral::ITable& dest = session->schema(schema).tableHandle( dest_table );
  std::unique_ptr< coral::IOperationWithQuery > op(dest.dataEditor().insertWithQuery());
  coral::IQueryDefinition& query = op->query();
  query.addToTableList( src_table );

  if(!cond.empty()) {
    query.setCondition( cond, coral::AttributeList() );
  }

  long numberOfRowsCopied = op->execute();

  VerboseMsg3("  * copy " << numberOfRowsCopied << " rows from table " << src_table <<
              " to table " << dest_table << " using condition \'" << cond << '\'');
}

static void
copy_temp_data(int data_version,
            coral::ISessionProxy * session,
            const std::string& schema,
	    int verbose_level)
{
  std::string condition;

  if(data_version) {
    std::ostringstream s;
    s << "DATAVERSION != " << data_version;
    condition = s.str();
  }

  copy_table("TEMPOKSOBJECT", "OKSOBJECT", condition, session, schema, verbose_level);
  copy_table("TEMPOKSDATAREL", "OKSDATAREL", condition, session, schema, verbose_level);
  copy_table("TEMPOKSDATAVAL", "OKSDATAVAL", condition, session, schema, verbose_level);
}

InsertedDataDetails
put_data(OksKernel& kernel,
            OksObject::FSet * data_objects,
	    coral::ISessionProxy * session,
            const std::string& schema,
            int& schema_version,
            int& data_version,
            int base_version,
	    const std::string& description,
            long inc_threshold,
	    int verbose_level)
{
  int schema_version_in = schema_version;

  VerboseMsg1("Put data schema-version: \'" << schema_version <<
              "\' data-version: \'" << data_version <<
              "\' base-version: \'" << base_version << "\'...");

  coral::ITable& tableOksData = session->schema(schema).tableHandle( "OKSDATA" );

  if(schema_version == 0) {
    VerboseMsg2("  Detecting schema version...");
    schema_version = oks::ral::get_head_schema_version(session, schema, 0, verbose_level - 1, "  ");

    if(schema_version <= 0) {
      std::ostringstream text;
      text << "Cannot get HEAD schema version for release " << s_release << '.';
      throw std::runtime_error( text.str().c_str() );
    }

    VerboseMsg2("   * use schema version " << schema_version);
  }


  if(data_version == 0) {
    VerboseMsg2("  Detecting data version...");
    get_data_version(session, schema, "", schema_version, data_version, 0, verbose_level - 1, "  ");

    if(schema_version <= 0) {
      std::ostringstream text;
      text << "Cannot get schema version " << schema_version_in << '.';
      throw std::runtime_error( text.str().c_str() );
    }

    if(data_version <= 0) {
      data_version = 1;
      VerboseMsg2("   * use version 1 to put first data of schema version " << schema_version);
    }
    else {
      data_version++;
      VerboseMsg2("   * use data version " << data_version);
    }
  }

  if(base_version == 0) {
    VerboseMsg2("  Detecting base version...");
    get_data_version(session, schema, "", schema_version, base_version, 0, verbose_level - 1, "  ");

    if(schema_version <= 0) {
      std::ostringstream text;
      text << "Cannot get schema version " << schema_version_in << '.';
      throw std::runtime_error( text.str() );
    }

    if(base_version <= 0) {
      throw std::runtime_error( "Cannot get base data version." );
    }
    else {
      VerboseMsg2("   * use base version " << base_version);
    }
  }


  int real_base_version = base_version;


    // get info on base version

  if(base_version > 0) {
    VerboseMsg2("Testing base data version " << base_version << "...");

    std::unique_ptr< coral::IQuery > query( tableOksData.newQuery() );
    query->setRowCacheSize( 1 );
    query->addToOutputList( "BASEVERSION");
    coral::AttributeList conditionData;
    conditionData.extend<int>( "dv" );
    conditionData.extend<int>( "sv" );
    conditionData["dv"].data<int>() = base_version;
    conditionData["sv"].data<int>() = schema_version;
    query->setCondition( "VERSION = :dv AND SCHEMAVERSION = :sv", conditionData );

    coral::ICursor& cursor = query->execute();

    int v = -1;

    if( cursor.next() ) {
      v = cursor.currentRow()["BASEVERSION"].data<int>();
    }

    if(v < 0) {
      std::ostringstream text;
      text << "Cannot get data for schema " << schema_version << " and base data " << base_version << " versions.";
      throw std::runtime_error( text.str() );
    }

    if(v != base_version) {
      base_version = v;
      VerboseMsg2(" * the actual base version is " << base_version);
    }
  }

  OksClass * class2 = 0;
  int position;


    // load schema or base configuration

  std::unique_ptr< ::OksKernel > k2 ( new ::OksKernel() );

  {
    VerboseMsg2("Check used schema...");

    std::string f1(k2->get_tmp_file("/tmp/temp.schema.xml"));
    OksFile * h1 = 0;

    try {
      h1 = k2->new_schema(f1);
    }
    catch (oks::exception & ex) {
      std::ostringstream text;
      text << "Cannot create temporal file \'" << f1 << "\' to read oks schema: " << ex.what();
      throw std::runtime_error( text.str() );
    }

    OksFile * h2 = 0;
    
    if(base_version > 0) {
      VerboseMsg2("Loading base data...");
      std::string f2(k2->get_tmp_file("/tmp/temp.data.xml"));

      try {
        h2 = k2->new_data(f2);
      }
      catch (oks::exception & ex) {
        std::ostringstream text;
        text << "Cannot create temporal file \'" << f2 << "\' to read data of base version: " << ex.what();
        throw std::runtime_error( text.str() );
      }

      get_data(*k2, session, schema, schema_version, base_version, verbose_level, &kernel.classes());

      try {
        h2->unlock();
        unlink(f2.c_str());
      }
      catch(oks::exception& ex) {
        throw std::runtime_error( ex.what() );
      }
    }
    else {
      VerboseMsg2("Loading schema...");
      base_version = data_version;
      get_schema(*k2, session, schema, schema_version, false, verbose_level);
    }

    try {
      h1->unlock();
      unlink(f1.c_str());
    }
    catch(oks::exception& ex) {
      throw std::runtime_error( ex.what() );
    }

    VerboseMsg2("Compare schema...");

    OksClass::Map new_classes;

    for(OksClass::Map::const_iterator class_it = kernel.classes().begin(); class_it != kernel.classes().end(); ++class_it) {
      if(OksClass * c = k2->find_class(class_it->second->get_name())) {
        if(c->compare_without_methods(*class_it->second) == false) {
          std::ostringstream text;
          text
	    << "Class \'" << class_it->second->get_name() << "\' stored in file \'" << class_it->second->get_file()->get_full_file_name()
	    << "\' differs from one stored in database (schema version " << schema_version << ')';
          throw std::runtime_error( text.str() );
        }
      }
      else {
        new_classes[class_it->second->get_name().c_str()] = class_it->second;
        VerboseMsg2(" * class \'" << class_it->second->get_name() << "\' from \'" << class_it->second->get_file()->get_full_file_name() << "\' to be added to schema version " << schema_version);
      }
    }

    if(new_classes.size() > 0) {
      VerboseMsg2("  Importing " << new_classes.size() << " new classes...");
      put_schema(new_classes, session, schema, -schema_version, "", verbose_level);
    }
  }


  VerboseMsg2("  Importing data...");

    // create data

  {
    VerboseMsg3("    * creating row in OksData table...");

    coral::AttributeList data;
    tableOksData.dataEditor().rowBuffer( data );

    data["SCHEMAVERSION"].data<int>() = schema_version;
    data["BASEVERSION"].data<int>() = base_version;
    data["VERSION"].data<int>() = data_version;
    data["DESCRIPTION"].data<std::string>() = description;
    set_time_host_user(data);


    try {
      tableOksData.dataEditor().insertRow( data );
    }
    catch ( coral::SchemaException& e ) {
      throw_schema_exception(e, "OksData");
    }
  }


    // create objects

  {
    InsertedDataDetails rv;
  
      // get OksObject table

    coral::ITable& tableOksObject = session->schema(schema).tableHandle( "TEMPOKSOBJECT" );
    coral::AttributeList dataOksObject;
    tableOksObject.dataEditor().rowBuffer( dataOksObject );
    dataOksObject["SCHEMAVERSION"].data<int>() = schema_version;
    dataOksObject["CREATIONVERSION"].data<int>() = data_version;


      // initialize OksDataRel bulk inserter

    coral::ITable& tableOksDataRel = session->schema(schema).tableHandle( "TEMPOKSDATAREL" );
    coral::AttributeList dataOksDataRel;
    coral::ITableDataEditor& dataOksDataRelEditor = tableOksDataRel.dataEditor();
    dataOksDataRelEditor.rowBuffer( dataOksDataRel );
    dataOksDataRel["SCHEMAVERSION"].data<int>() = schema_version;
    dataOksDataRel["CREATIONVERSION"].data<int>() = data_version;
    std::unique_ptr< coral::IBulkOperation > inserterOksDataRel ( dataOksDataRelEditor.bulkInsert( dataOksDataRel, 1000 ) );


      // initialize OksDataVal bulk inserter

    coral::ITable& tableOksDataVal = session->schema(schema).tableHandle( "TEMPOKSDATAVAL" );
    coral::AttributeList dataOksDataVal;
    coral::ITableDataEditor& dataOksDataValEditor = tableOksDataVal.dataEditor();

//    dataOksDataValEditor.rowBuffer( dataOksDataVal );

    dataOksDataVal.extend<int>("SCHEMAVERSION");
    dataOksDataVal.extend<int>("DATAVERSION");
    dataOksDataVal.extend<int>("CREATIONVERSION");
    dataOksDataVal.extend<std::string>("CLASSNAME");
    dataOksDataVal.extend<std::string>("OBJECTID");
    dataOksDataVal.extend<std::string>("NAME");
    dataOksDataVal.extend<int>("PARENTPOSITION");
    dataOksDataVal.extend<int64_t>("INTEGERVALUE");
    dataOksDataVal.extend<double>("NUMBERVALUE");
    dataOksDataVal.extend<std::string>("STRINGVALUE");

    dataOksDataVal["SCHEMAVERSION"].data<int>() = schema_version;
    dataOksDataVal["CREATIONVERSION"].data<int>() = data_version;
    std::unique_ptr< coral::IBulkOperation > inserterOksDataVal ( dataOksDataValEditor.bulkInsert( dataOksDataVal, 1000 ) );


      // create objects

    for(OksClass::Map::const_iterator class_it = kernel.classes().begin(); class_it != kernel.classes().end(); ++class_it) {
      VerboseMsg3(" * process objects of class \'" << class_it->second->get_name() << '\'');
      if(base_version > 0) {
        class2 = k2->find_class(class_it->second->get_name());
      }
      if(const OksObject::Map * objects = class_it->second->objects()) {
	dataOksObject["CLASSNAME"].data<std::string>() = class_it->second->get_name();
        for(OksObject::Map::const_iterator i = objects->begin(); i != objects->end(); ++i) {
          OksObject * o = i->second;
	  short state = 1;  // created

          if( (data_objects != 0) && (data_objects->find(o) == data_objects->end()) ) {
	    VerboseMsg4("   - skip " << o << " (it is not in the set of objects)");
	    continue;
	  }

          if(o->is_duplicated() == true) {
            VerboseMsg4("   - skip " << o << " (it is a duplicated object)");
            continue;
          }

          bool inserted_to_base = false;

          int object_version = data_version;

	  if(base_version > 0) {
	    if(OksObject * o2 = (class2 ? class2->get_object(o->GetId()) : 0)) {
	      if(*o == *o2) {
	        VerboseMsg4("   - unmodified object " << o << " is found in base configuration");
	        continue;
	      }
	      else {
		VerboseMsg4("   - modified object " << o << " is found in base configuration");
	        state = 2; // object is updated
	      }
	    }
	    else {
	      VerboseMsg4("   - object " << o << " is not found in base configuration " << base_version);
              object_version = base_version;
	      inserted_to_base = true;
	    }
	  }

          dataOksObject["DATAVERSION"].data<int>() = object_version;
          VerboseMsg4("   - add " << (state == 1 ? "new" : "updated") << " object " << o << " to version " << object_version);

	  dataOksObject["ID"].data<std::string>() = o->GetId();
	  dataOksObject["STATE"].data<short>() = state;

          try {
            tableOksObject.dataEditor().insertRow( dataOksObject );
	    if(inserted_to_base) { rv.m_insertedOksObjectRowsBase++; }
	    else                 { rv.m_insertedOksObjectRowsThis++; }
          }
          catch ( coral::SchemaException& e ) {
            throw_schema_exception(e, "OksObject");
          }
        }
      }
    }
    
      // reset in case if last object was created in base version

    dataOksObject["DATAVERSION"].data<int>() = data_version;

    
      // mark objects to be removed

    if(base_version > 0 && data_objects == 0) {
      for(OksClass::Map::const_iterator class_it2 = k2->classes().begin(); class_it2 != k2->classes().end(); ++class_it2) {
        VerboseMsg3(" * test objects of class \'" << class_it2->second->get_name() << '\'');

        OksClass * c = kernel.find_class(class_it2->second->get_name());

        if(c == 0) {
          VerboseMsg4("   - skip objects of class " << class_it2->second->get_name() << " (no such class in incremented version)");
          continue;
        }

        if(const OksObject::Map * objects = class_it2->second->objects()) {
	  for(OksObject::Map::const_iterator i = objects->begin(); i != objects->end(); ++i) {
            OksObject * o = i->second;

            if(o->is_duplicated() == true) {
              VerboseMsg4("   - skip " << o << " (it is a duplicated object)");
              continue;
            }

	    if(!c->get_object(o->GetId())) {
              VerboseMsg4("   - remove object \'" << o->GetId() << '\'');

	      dataOksObject["CLASSNAME"].data<std::string>() = c->get_name();
	      dataOksObject["ID"].data<std::string>() = o->GetId();
	      dataOksObject["STATE"].data<short>() = 0;

              try {
                tableOksObject.dataEditor().insertRow( dataOksObject );
	        rv.m_insertedOksObjectRowsThis++;
              }
              catch ( coral::SchemaException& e ) {
                throw_schema_exception(e, "OksObject");
              }
	    }
	  }
	}
      }
    }


    for(OksClass::Map::const_iterator class_it = kernel.classes().begin(); class_it != kernel.classes().end(); ++class_it) {
      VerboseMsg3(" * process class \'" << class_it->second->get_name() << '\'');
      if(base_version > 0) {
        class2 = k2->find_class(class_it->second->get_name());
      }

      if((class_it->second->number_of_all_attributes() + class_it->second->number_of_all_relationships()) > 0) {
        if(const OksObject::Map * objects = class_it->second->objects()) {
	  OksData * d = 0;
	  OksData * d2 = 0;
	  OksDataInfo odi(0, (OksAttribute *)0);
          const std::list<OksAttribute *> * alist = class_it->second->all_attributes();
          const std::list<OksRelationship *> * rlist = class_it->second->all_relationships();

          dataOksDataVal["CLASSNAME"].data<std::string>() = class_it->second->get_name();
          dataOksDataRel["CLASSNAME"].data<std::string>() = class_it->second->get_name();

          for(OksObject::Map::const_iterator i = objects->begin(); i != objects->end(); ++i) {
            OksObject * o = i->second;

            if( (data_objects != 0) && (data_objects->find(o) == data_objects->end()) ) {
	      VerboseMsg4("   - skip " << o << " (it is not in the set of objects)");
	      continue;
	    }

            if(o->is_duplicated() == true) {
              VerboseMsg4("   - skip " << o << " (it is a duplicated object)");
              continue;
            }

	    OksObject * o2 = (((base_version > 0) && class2) ? class2->get_object(o->GetId()) : 0);
            bool inserted_to_base = false;

            if(o2) {
              dataOksDataRel["DATAVERSION"].data<int>() = data_version;
              dataOksDataVal["DATAVERSION"].data<int>() = data_version;
              VerboseMsg4("   - check attributes and relationships of object \'" << o->GetId() << '\'');
	    }
	    else {
	      if(base_version) {
                dataOksDataRel["DATAVERSION"].data<int>() = base_version;
                dataOksDataVal["DATAVERSION"].data<int>() = base_version;
		inserted_to_base = true;
                VerboseMsg4("   - put attributes and relationships of object \'" << o->GetId() << "\' into base version " << base_version);
	      }
	      else {
                dataOksDataRel["DATAVERSION"].data<int>() = data_version;
                dataOksDataVal["DATAVERSION"].data<int>() = data_version;
                VerboseMsg4("   - put attributes and relationships of object \'" << o->GetId() << '\'');
	      }
	    }

	    odi.offset = 0;
            if( alist && !alist->empty() ) {
              dataOksDataVal["OBJECTID"].data<std::string>() = o->GetId();
              for(std::list<OksAttribute *>::const_iterator ai = alist->begin(); ai != alist->end(); ++ai) {
	        d = o->GetAttributeValue(&odi);
		d2 = o2 ? o2->GetAttributeValue(&odi) : 0;
	        odi.offset++;
		if((d2 != 0) && (*d2 == *d)) continue;

                if(o2) {
                  VerboseMsg5("     # values of attribute \'" << (*ai)->get_name() << "\' are different (" << *d << " vs. " << *d2 << ")");
                }

                dataOksDataVal["NAME"].data<std::string>() = (*ai)->get_name();

		if(d->type == OksData::list_type) {
                  bool mark_empty = (d2 && d2->data.LIST && !d2->data.LIST->empty());
		  if(const OksData::List * dlist = d->data.LIST) {
		    position = 1;
		    for(OksData::List::const_iterator li = dlist->begin(); li != dlist->end(); ++li) {
		      fill_attr_value(dataOksDataVal, inserterOksDataVal.get(), **li, position++);
		      if(inserted_to_base) { rv.m_insertedOksDataValRowsBase++; } else { rv.m_insertedOksDataValRowsThis++; }
                      mark_empty = false;
		    }
		  }
                  if(mark_empty) {
                    OksData d3((int32_t)0);
                    fill_attr_value(dataOksDataVal, inserterOksDataVal.get(), d3, 0);
                    if(inserted_to_base) { rv.m_insertedOksDataValRowsBase++; } else { rv.m_insertedOksDataValRowsThis++; }
                    VerboseMsg5("     # mark attribute \'" << (*ai)->get_name() << "\' of \'" << o->GetId() << "\' as empty");
                  }
		}
		else {
		  fill_attr_value(dataOksDataVal, inserterOksDataVal.get(), *d, 0);
		  if(inserted_to_base) { rv.m_insertedOksDataValRowsBase++; } else { rv.m_insertedOksDataValRowsThis++; }
		}
              }
            }
            if( rlist && !rlist->empty() ) {
              dataOksDataRel["OBJECTID"].data<std::string>() = o->GetId();
              for(std::list<OksRelationship *>::const_iterator ri = rlist->begin(); ri != rlist->end(); ++ri) {
	        d = o->GetRelationshipValue(&odi);
		d2 = o2 ? o2->GetRelationshipValue(&odi) : 0;
	        odi.offset++;
		if((d2 != 0) && (*d2 == *d)) continue;

                dataOksDataRel["NAME"].data<std::string>() = (*ri)->get_name();
		
		int dv = (inserted_to_base ? base_version : data_version);

		if(d->type == OksData::list_type) {
                  bool mark_empty = (d2 && d2->data.LIST && !d2->data.LIST->empty());
		  if(const OksData::List * dlist = d->data.LIST) {
		    position = 1;
		    for(OksData::List::const_iterator li = dlist->begin(); li != dlist->end(); ++li) {
		      create_rel_value(dataOksDataRel, inserterOksDataRel.get(), dv, **li, position++, (base_version > 0) ? k2.get() : 0, base_version);
		      if(inserted_to_base) { rv.m_insertedOksDataRelRowsBase++; } else { rv.m_insertedOksDataRelRowsThis++; }
                      mark_empty = false;
		    }
		  }
                  if(mark_empty) {
                    OksData d3(o);
                    create_rel_value(dataOksDataRel, inserterOksDataRel.get(), dv, d3, 0, 0, base_version);
		    if(inserted_to_base) { rv.m_insertedOksDataRelRowsBase++; } else { rv.m_insertedOksDataRelRowsThis++; }
                    VerboseMsg5("     # mark relationship \'" << (*ri)->get_name() << "\' of \'" << o->GetId() << "\' as empty");
                  }
		}
		else {
		  create_rel_value(dataOksDataRel, inserterOksDataRel.get(), dv, *d, 0, (base_version > 0) ? k2.get() : 0, base_version);
		  if(inserted_to_base) { rv.m_insertedOksDataRelRowsBase++; } else { rv.m_insertedOksDataRelRowsThis++; }
		}
              }
            }
          }
        }
      }
    }

    inserterOksDataVal->flush();
    inserterOksDataRel->flush();

    VerboseMsg2("Number of inserted rows by tables into incremental version " << data_version << ":\n"
                "  oks objects         : " << rv.m_insertedOksObjectRowsThis << "\n"
                "  relationship values : " << rv.m_insertedOksDataRelRowsThis << "\n"
	        "  attribute values    : " << rv.m_insertedOksDataValRowsThis << "\n"
                "Number of inserted rows by tables into base version " << base_version << ":\n"
                "  oks objects         : " << rv.m_insertedOksObjectRowsBase << "\n"
                "  relationship values : " << rv.m_insertedOksDataRelRowsBase << "\n"
	        "  attribute values    : " << rv.m_insertedOksDataValRowsBase );

    int ret_value1 = rv.m_insertedOksObjectRowsThis + rv.m_insertedOksDataRelRowsThis + rv.m_insertedOksDataValRowsThis;
    int ret_value2 = rv.m_insertedOksObjectRowsBase + rv.m_insertedOksDataRelRowsBase + rv.m_insertedOksDataValRowsBase;


        // if there are no differences in incremental version

    if(real_base_version > 0 && ret_value1 == 0) {
      if(ret_value2 == 0) {
        VerboseMsg2("  There are no differences from base version " << base_version);
        data_version = base_version;
        rv.m_use_base = true;
        return rv;
      }
      else {
        VerboseMsg2("  The incremental version has no differences; there are changes in base version")
	copy_temp_data(0, session, schema, verbose_level);
        return rv;
      }
    }


        // check if there are differences from incremental versions

    if(real_base_version > 0 && real_base_version != base_version) {

      std::vector<int> vers;

      vers.push_back(real_base_version);

        // try to find last versions for this partition

      {

        static int __max_num_of_part_versions_for_test = 0;

	if(__max_num_of_part_versions_for_test == 0) {
	  const char * v(::getenv("R_OKS_MAX_NUM_OF_PART_VERSIONS_FOR_TEST"));
	  if(v && *v) {
	    __max_num_of_part_versions_for_test = atoi(v);
	    if(__max_num_of_part_versions_for_test < 1) __max_num_of_part_versions_for_test = 1;
	  }
	  else {
	    __max_num_of_part_versions_for_test = 8;
	  }
          VerboseMsg2("  Initialize max_num_of_part_versions_for_test param = " << __max_num_of_part_versions_for_test );
	}

        std::string partition_name;

	const char pattern[] = "partition ";

        std::string::size_type idx = description.find(pattern);
	if(idx != std::string::npos) {
	  idx += sizeof(pattern) - 1;
	  std::string::size_type idx2 = description.find(' ', idx);
	  if(idx2 != std::string::npos) {
	    partition_name = description.substr(idx, idx2-idx);
            VerboseMsg2("  Looking for recently archived versions for partition \'" << partition_name << "\'...");
	  }
	}

        if(!partition_name.empty()) {
          std::unique_ptr< coral::IQuery > query( session->schema(schema).tableHandle("OKSARCHIVE").newQuery() );
          query->setRowCacheSize( 10 );
          query->addToOutputList( "DISTINCT DATAVERSION" );
	  query->addToOrderList( "DATAVERSION DESC" );
          coral::AttributeList conditionData;
          conditionData.extend<int>( "sv" );
          conditionData["sv"].data<int>() = schema_version;
          conditionData.extend<std::string>( "p" );
          conditionData["p"].data<std::string>() = partition_name;
          query->setCondition( "SCHEMAVERSION = :sv AND PARTITION = :p", conditionData );

          coral::ICursor& cursor = query->execute();

          while( cursor.next() && vers.size() <= (size_t)__max_num_of_part_versions_for_test ) {
	    int av = cursor.currentRow().begin()->data<int>();
            if(av != real_base_version) {
              VerboseMsg2("  * add archived version " << av);
	      vers.push_back(av);
	    }
          }
        }
      }

      const char * oks_obj_names[] = {
        "CLASSNAME",
        "ID",
        "STATE",
        0
      };

      const char * oks_data_names[] = {
        "CLASSNAME",
        "OBJECTID",
        "NAME",
        "PARENTPOSITION",
        "INTEGERVALUE",
        "NUMBERVALUE",
        "STRINGVALUE",
        0
      };

      const char * oks_rel_names[] = {
        "CLASSNAME",
        "OBJECTID",
        "NAME",
        "PARENTPOSITION",
        "VALUECLASSNAME",
        "VALUEOBJECTID",
        0
      };

      for(unsigned int j = 0; j < vers.size(); ++j) {
        VerboseMsg2("  Testing differences between base data version " << vers[j] << " and new data version " << data_version << " ...")

        std::ostringstream cond1;
        cond1 << "SCHEMAVERSION = " << schema_version << " AND DATAVERSION = " << data_version;

        std::ostringstream cond2;
        cond2 << "SCHEMAVERSION = " << schema_version << " AND DATAVERSION = " << vers[j];

        if(
            // check if there are differences between objects

          compare_2_tables(session, schema, "TEMPOKSOBJECT", "OKSOBJECT", oks_obj_names, cond1.str(), cond2.str(), vers[j], data_version, verbose_level) == 0 &&
          compare_2_tables(session, schema, "OKSOBJECT", "TEMPOKSOBJECT", oks_obj_names, cond2.str(), cond1.str(), vers[j], data_version, verbose_level) == 0 &&

            // check if there are differences between attributes of objects

          compare_2_tables(session, schema, "TEMPOKSDATAVAL", "OKSDATAVAL", oks_data_names, cond1.str(), cond2.str(), vers[j], data_version, verbose_level) == 0 &&
          compare_2_tables(session, schema, "OKSDATAVAL", "TEMPOKSDATAVAL", oks_data_names, cond2.str(), cond1.str(), vers[j], data_version, verbose_level) == 0 &&

            // check if there are differences between relationships of objects

          compare_2_tables(session, schema, "TEMPOKSDATAREL", "OKSDATAREL", oks_rel_names, cond1.str(), cond2.str(), vers[j], data_version, verbose_level) == 0 &&
          compare_2_tables(session, schema, "OKSDATAREL", "TEMPOKSDATAREL", oks_rel_names, cond2.str(), cond1.str(), vers[j], data_version, verbose_level) == 0
	) {
          if(ret_value2 == 0) {
            VerboseMsg2("  The version " << vers[j] << " and new data version " << data_version << " have no differences; no changes in base version")
            data_version = vers[j];
	    rv.m_use_base = true;
          }
          else {
            std::cout << "IMPORTANT CASE: The version " << vers[j] << " and new data version " << data_version << " have no differences; there are changes in base version!" << std::endl;
            VerboseMsg1("  The version " << vers[j] << " and new data version " << data_version << " have no differences; there are changes in base version")
            data_version = vers[j];
	    copy_temp_data(data_version, session, schema, verbose_level); // ignore data with (version == data_version)
          }

          return rv;
	}
	else {
          VerboseMsg2("  Found differences between base data version " << vers[j] << " and new data version " << data_version << " ...")
	}
      }

      VerboseMsg2("  New data version " << data_version << " needs to be created")
    }

    if(inc_threshold <= 0 || inc_threshold >= ret_value1) {
      copy_temp_data(0, session, schema, verbose_level);
    }
    else {
      std::cout << "  The incremental version size " << ret_value1 << " exceeds threshold limit equal " << inc_threshold << " rows. The data are not copied from temporal tables and must not be committed." << std::endl;
    }

    return rv;
  }

}


////////////////////////////////////////////////////////////////////////////////

static void
throw_cannot_find_class(const std::string& name)
{
  std::ostringstream s;
  s << "Internal error: failed to find class \'" << name << "\'.";
  throw std::runtime_error( s.str() );
}

static void
throw_cannot_find_object(const std::string& cname, const std::string& obj_id)
{
  std::ostringstream s;
  s << "Internal error: failed to find object \'" << obj_id << "\' in class \'" << cname << "\'.";
  throw std::runtime_error( s.str() );
}

static void
throw_cannot_find_object_param(const char * what, const std::string& name, const OksObject * o)
{
  std::ostringstream s;
  s << "Internal error: failed to find " << what << " \'" << name << "\' when read object " << o << '.';
  throw std::runtime_error( s.str() );
}



////////////////////////////////////////////////////////////////////////////////
///                                GET SCHEMA                                ///
////////////////////////////////////////////////////////////////////////////////

void
get_schema(OksKernel& kernel,
	    coral::ISessionProxy * session,
            const std::string& schema,
	    int schema_version,
	    bool read_methods,
	    int verbose_level)
{
  VerboseMsg1("Get schema " << schema_version << "...");

  coral::ITable& tableOksSchema = session->schema(schema).tableHandle( "OKSSCHEMA" );

    // get schema description

  if(verbose_level > 1) {
    std::cout << "  Reading schema description..." << std::endl;

    std::unique_ptr< coral::IQuery > query( tableOksSchema.newQuery() );
    coral::AttributeList conditionData;
    conditionData.extend<int>( "ver" );
    conditionData["ver"].data<int>() = schema_version;
    query->setCondition( "VERSION = :ver", conditionData );

    coral::ICursor& cursor = query->execute();

    if( cursor.next() ) {
      const coral::AttributeList& row = cursor.currentRow();
      std::string time, host, user;
      boost::local_time::time_zone_ptr tz_ptr; // do not init => use UTC
      get_time_host_user(row, time, host, user, tz_ptr);

      std::cout << "    Version     : " << row["VERSION"].data<int>() << "\n"
                   "    Release     : " << row["RELEASE"].data<std::string>() << "\n"
                   "    Created on  : " << time << " (UTC)\n"
                   "    Created at  : " << host << "\n"
                   "    Created by  : " << user << "\n"
	           "    Description : " << row["DESCRIPTION"].data<std::string>() << std::endl;	  
    }
  }


  coral::AttributeList versionConditionData;
  versionConditionData.extend<int>( "ver" );
  versionConditionData["ver"].data<int>() = schema_version;

  coral::AttributeList versionAndDirectConditionData;
  versionAndDirectConditionData.extend<int>( "ver" );
  versionAndDirectConditionData.extend<int>( "dir" );
  versionAndDirectConditionData["ver"].data<int>() = schema_version;
  versionAndDirectConditionData["dir"].data<int>() = 1;


    // get classes

  {
    VerboseMsg2("  Reading oks classes...");

    coral::ITable& table = session->schema(schema).tableHandle( "OKSCLASS" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );
    query->setRowCacheSize( 100 );
    query->addToOutputList( "NAME");
    query->addToOutputList( "DESCRIPTION");
    query->addToOutputList( "ISABSTRACT");
    query->setCondition( "VERSION = :ver", versionConditionData );

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      std::string name = iColumn->data<std::string>(); ++iColumn;
      std::string desc; if( iColumn->isNull() == false ) { desc = iColumn->data<std::string>(); } ++iColumn;
      bool is_abstract = iColumn->data<bool>();
      new OksClass(name, desc, is_abstract, &kernel);
      VerboseMsg3("   * get class \'" << name << '\'');
    }
  }


      // get super-classes

  {
    VerboseMsg2("  Reading oks classes inheritance hierarchy...");

    coral::ITable& table = session->schema(schema).tableHandle( "OKSSUPERCLASS" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );
    query->setRowCacheSize( 200 );
    query->addToOrderList( "PARENTNAME" );
    query->addToOrderList( "PARENTPOSITION" );
    query->addToOutputList( "PARENTNAME");
    query->addToOutputList( "VALUE");

    query->setCondition( "VERSION = :ver", versionConditionData );

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      if(OksClass * c  = kernel.find_class(iColumn->data<std::string>())) {
	++iColumn;
        try {
	  c->add_super_class(iColumn->data<std::string>());
        }
        catch(oks::exception& ex) {
          std::ostringstream text;
          text << "Internal error: " << ex.what();
          throw std::runtime_error( text.str() );
        }

	VerboseMsg3("   * add class \'" << iColumn->data<std::string>() << "\' as base class of \'" << c->get_name() << '\'');
      }
      else {
        throw_cannot_find_class(iColumn->data<std::string>());
      }
    }
  }


      // get attributes

  {
    VerboseMsg2("  Reading attributes of oks classes...");

    coral::ITable& table = session->schema(schema).tableHandle( "OKSATTRIBUTE" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );

    query->setRowCacheSize( 1000 );

    query->addToOrderList( "PARENTNAME" );
    query->addToOrderList( "PARENTPOSITION" );

    query->addToOutputList( "PARENTNAME");
    query->addToOutputList( "NAME");
    query->addToOutputList( "DESCRIPTION");
    query->addToOutputList( "TYPE");
    query->addToOutputList( "RANGE");
    query->addToOutputList( "FORMAT");
    query->addToOutputList( "ISMULTIVALUE");
    query->addToOutputList( "INITVALUE");
    query->addToOutputList( "ISNOTNULL");

    query->setCondition( "VERSION = :ver AND ISDIRECT = :dir", versionAndDirectConditionData );

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
        if(OksClass * c  = kernel.find_class(iColumn->data<std::string>())) {
	++iColumn;
	std::string name = iColumn->data<std::string>(); ++iColumn;
	std::string desc; if( iColumn->isNull() == false ) { desc = iColumn->data<std::string>(); } ++iColumn;
	std::string type = iColumn->data<std::string>(); ++iColumn;
	std::string range; if( iColumn->isNull() == false ) { range = iColumn->data<std::string>(); } ++iColumn;
	std::string format; if( iColumn->isNull() == false ) { format = iColumn->data<std::string>(); } ++iColumn;
	bool is_mv = iColumn->data<bool>(); ++iColumn;
	std::string init; if( iColumn->isNull() == false ) { init = iColumn->data<std::string>(); } ++iColumn;
	bool is_not_null= iColumn->data<bool>(); // not used for the moment

        try {
          c->add(
            new OksAttribute(
              name, type, is_mv, range, init, desc, is_not_null,
              OksAttribute::str2format(format.c_str())
            )
          );
        }
        catch(oks::exception& ex) {
          std::ostringstream text;
          text << "Internal error: failed to add attribute \'" << name << "\' to class \'" << c->get_name() << "\' caused by:\n" << ex.what();
          throw std::runtime_error( text.str() );
        }

	VerboseMsg3("   * get attribute \'" << name << "\' of class \'" << c->get_name() << '\'');
      }
      else {
	throw_cannot_find_class(iColumn->data<std::string>());
      }
    }
  }


    // get relationships

  {
    VerboseMsg2("  Reading relationships between oks classes...");

    coral::ITable& table = session->schema(schema).tableHandle( "OKSRELATIONSHIP" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );

    query->setRowCacheSize( 1000 );
 
    query->addToOrderList( "PARENTNAME" );
    query->addToOrderList( "PARENTPOSITION" );

    query->addToOutputList( "PARENTNAME");
    query->addToOutputList( "NAME");
    query->addToOutputList( "DESCRIPTION");
    query->addToOutputList( "CLASSTYPE");
    query->addToOutputList( "LOWCC");
    query->addToOutputList( "HIGHCC");
    query->addToOutputList( "ISCOMPOSITE");
    query->addToOutputList( "ISEXCLUSIVE");
    query->addToOutputList( "ISDEPENDENT");

    query->setCondition( "VERSION = :ver AND ISDIRECT = :dir", versionAndDirectConditionData );

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      if(OksClass * c = kernel.find_class(iColumn->data<std::string>())) {
        ++iColumn;
        std::string name = iColumn->data<std::string>(); ++iColumn;
        std::string desc; if( iColumn->isNull() == false ) { desc = iColumn->data<std::string>(); } ++iColumn;
        std::string type = iColumn->data<std::string>(); ++iColumn;
        std::string low_cc = iColumn->data<std::string>(); ++iColumn;
        std::string high_cc = iColumn->data<std::string>(); ++iColumn;
        bool is_comp = iColumn->data<bool>(); ++iColumn;
        bool is_excl = iColumn->data<bool>(); ++iColumn;
        bool is_dep = iColumn->data<bool>(); ++iColumn;

        try {
          c->add(
            new OksRelationship(
              name, type,
              OksRelationship::str2card(low_cc.c_str()),
              OksRelationship::str2card(high_cc.c_str()),
              is_comp, is_excl, is_dep,
              desc
            )
          );
        }
        catch(oks::exception& ex) {
          std::ostringstream text;
          text << "Internal error: failed to add relationship \'" << name << "\' to class \'" << c->get_name() << "\' caused by:\n" << ex.what();
          throw std::runtime_error( text.str() );
        }

	VerboseMsg3("   * get relationship \'" << name << "\' of class \'" << c->get_name() << '\'');
      }
      else {
	throw_cannot_find_class(iColumn->data<std::string>());
      }
    }
  }


    // get methods

  if(read_methods) {
    VerboseMsg2("  Reading methods of oks classes...");

    coral::ITable& table = session->schema(schema).tableHandle( "OKSMETHOD" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );

    query->setRowCacheSize( 1000 );
    query->addToOrderList( "PARENTNAME" );
    query->addToOrderList( "PARENTPOSITION" );
    query->addToOutputList( "PARENTNAME");
    query->addToOutputList( "NAME");
    query->addToOutputList( "DESCRIPTION");

    query->setCondition( "VERSION = :ver AND ISDIRECT = :dir", versionAndDirectConditionData );

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      if(OksClass * c  = kernel.find_class(iColumn->data<std::string>())) {
	++iColumn;
	std::string name = iColumn->data<std::string>(); ++iColumn;
        std::string desc; if( iColumn->isNull() == false ) { desc = iColumn->data<std::string>(); }

        try {
          c->add(new OksMethod(name, desc));
        }
        catch(oks::exception& ex) {
          std::ostringstream text;
          text << "Internal error: failed to add method \'" << name << "\' to class \'" << c->get_name() << "\' caused by:\n" << ex.what();
          throw std::runtime_error( text.str() );
        }

	VerboseMsg3("   * get method \'" << name << "\' of class \'" << c->get_name() << '\'');
      }
      else {
	throw_cannot_find_class(iColumn->data<std::string>());
      }
    }
  }


    // get method implementations

  if(read_methods) {
    VerboseMsg2("  Reading implementations of the methods...");

    coral::ITable& table = session->schema(schema).tableHandle( "OKSMETHODIMPLEMENTATION" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );

    query->setRowCacheSize( 1000 );

    query->addToOrderList( "PARENTCLASSNAME" );
    query->addToOrderList( "PARENTMETHODNAME" );
    query->addToOrderList( "PARENTPOSITION" );

    query->addToOutputList( "PARENTCLASSNAME");
    query->addToOutputList( "PARENTMETHODNAME");
    query->addToOutputList( "LANGUAGE");
    query->addToOutputList( "PROTOTYPE");
    query->addToOutputList( "BODY");

    query->setCondition( "VERSION = :ver", versionConditionData );

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      if(OksClass * c  = kernel.find_class(iColumn->data<std::string>())) {
	++iColumn;
	if(OksMethod * m = c->find_method(iColumn->data<std::string>())) {
	  ++iColumn;
	  std::string lang = iColumn->data<std::string>(); ++iColumn;
	  std::string prt = iColumn->data<std::string>(); ++iColumn;
	  std::string body; if( iColumn->isNull() == false ) { body = iColumn->data<std::string>(); }

          try {
	    m->add_implementation(lang, prt, body);
          }
          catch(oks::exception& ex) {
            std::ostringstream text;
            text << "Internal error: failed to add implementation \'" << lang << "\' to method \'" << iColumn->data<std::string>() << "\' of class \'" << c->get_name() << "\' caused by:\n" << ex.what();
            throw std::runtime_error( text.str() );
          }

	  VerboseMsg3("   * get \'" << lang << "\' implementation of method \'" << m->get_name() << '@' << c->get_name() << '\'');
	}
	else {
	  std::ostringstream text;
          text << "Internal error: failed to find method \'" << iColumn->data<std::string>() << "\' of class \'" << c->get_name() << "\'.";
          throw std::runtime_error( text.str() );
	}
      }
      else {
	throw_cannot_find_class(iColumn->data<std::string>());
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
///                                 GET DATA                                 ///
////////////////////////////////////////////////////////////////////////////////


static void
set_value(const OksObject& o, const OksAttribute& a, coral::AttributeList::const_iterator& iColumn, int pos, bool is_str_null, int verbose_level)
{
  static const std::string empty;

  OksData * d(o.GetAttributeValue(a.get_name()));

  VerboseMsg5("   + set_attr " << &o << "[" << a.get_name() << "][" << pos << ']' );

  if(a.get_is_multi_values()) {
    if(pos == 0) {
      d->Set(new OksData::List());
      return;
    }

    if(d->data.LIST->size() < static_cast<size_t>(pos)) {
      OksData * at = 0;
      while(d->data.LIST->size() < static_cast<size_t>(pos)) {
        at = new OksData();
        d->data.LIST->push_back(at);
      }
      d = at;
    }
    else {
      OksData::List::iterator i = d->data.LIST->begin();
      while(pos-- > 1) ++i;
      d = *i;
    }

    if(d->type != OksData::unknown_type) {
      std::ostringstream text;
      text
        << "Internal error in set_value(" << &o << ", " << a.get_name() << ") at list.len = "
	<< d->data.LIST->size() << "]: the iterated value unexpectly is set to " << *d << '.';
      throw std::runtime_error( text.str() );
    }
  }

  switch(a.get_data_type()) {
    case OksData::string_type:
      ++iColumn; ++iColumn;
      d->Set(is_str_null ? empty : iColumn->data<std::string>());
      break;

    case OksData::enum_type:
      ++iColumn; ++iColumn;
      d->SetE((is_str_null ? empty : iColumn->data<std::string>()), &a);
      break;

    case OksData::s32_int_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((int32_t)v);
      break; }

    case OksData::u32_int_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((uint32_t)v);
      break; }

    case OksData::s16_int_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((int16_t)v);
      break; }

    case OksData::u16_int_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((uint16_t)v);
      break; }

    case OksData::u8_int_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((uint8_t)v);
      break; }

    case OksData::s8_int_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((int8_t)v);
      break; }

    case OksData::s64_int_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((int64_t)v);
      break; }

    case OksData::u64_int_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((uint64_t)v);
      break; }

    case OksData::bool_type: {
      int64_t v = iColumn->data<int64_t>(); d->Set((bool)v);
      break; }

    case OksData::float_type: {
      ++iColumn;
      double v = iColumn->data<double>(); d->Set((float)v);
      break; }

    case OksData::double_type: {
      ++iColumn;
      double v = iColumn->data<double>(); d->Set(v);
      break; }

    case OksData::date_type:
      ++iColumn; ++iColumn;
      d->Set(
        is_str_null
          ? boost::gregorian::day_clock::universal_day()
          : boost::gregorian::from_undelimited_string(iColumn->data<std::string>())
      );
      break;

    case OksData::time_type:
      ++iColumn; ++iColumn;
      d->Set(
        is_str_null
          ? boost::posix_time::second_clock::universal_time()
          : boost::posix_time::from_iso_string(iColumn->data<std::string>())
      );
      break;

    case OksData::class_type:
      ++iColumn; ++iColumn;
      try {
        d->data.CLASS = 0;
        d->type = OksData::class_type;
        const char * v = (is_str_null ? "" : iColumn->data<std::string>().c_str());
        d->SetValue(v, &a);
      }
      catch(oks::exception & ex) {
        std::ostringstream text;
        text << "set_value(obj: " << &o << ", attribute: " << a.get_name() << ") failed:\n" << ex.what();
        throw std::runtime_error( text.str() );
      }
      break;

    default: {
      std::ostringstream text;
      text << "Internal error: unexpected type of attribute \'" << a.get_name() << "\' of object " << &o << '.';
      throw std::runtime_error( text.str() ); }
  }
}

static bool
is_defined(const std::string& s1,  // class
           const std::string& s2,  // object-id
	   const std::map<std::string, std::set<std::string> >& m)
{
  std::map<std::string, std::set<std::string> >::const_iterator j = m.find(s1);
  return (j != m.end() && j->second.find(s2) != j->second.end());
}

static bool
is_defined(const std::string& s1,  // class
           const std::string& s2,  // object-id
           const std::string& s3,  // attribute or relationship name
	   const std::map<std::string, std::map<std::string, std::set<std::string> > >& m)
{
  std::map<std::string, std::map<std::string, std::set<std::string> > >::const_iterator j = m.find(s1);
  return (j != m.end() && is_defined(s2, s3, j->second));
}


unsigned long
get_data(OksKernel& kernel,
            coral::ISessionProxy * session,
            const std::string& schema,
            int schema_version,
            int data_version,
            int verbose_level,
            const OksClass::Map * pattern)
{
  int base_version = -1;

  VerboseMsg1("Get data for schema " << schema_version << " and data " << data_version << " versions...");

  {
    coral::ITable& table = session->schema(schema).tableHandle( "OKSDATA" );

    VerboseMsg2("  Reading the data description...");

    std::unique_ptr< coral::IQuery > query( table.newQuery() );
    query->setRowCacheSize( 1 );
    coral::AttributeList conditionData;
    conditionData.extend<int>( "sv" );
    conditionData.extend<int>( "dv" );
    conditionData["sv"].data<int>() = schema_version;
    conditionData["dv"].data<int>() = data_version;
    query->setCondition( "SCHEMAVERSION = :sv AND VERSION = :dv", conditionData );

    coral::ICursor& cursor = query->execute();

    if ( cursor.next() ) {
      const coral::AttributeList& row = cursor.currentRow();
      base_version = row["BASEVERSION"].data<int>();
      
      if(verbose_level > 1) {
        std::string time, host, user;
        boost::local_time::time_zone_ptr tz_ptr; // do not init => use UTC
        get_time_host_user(row, time, host, user, tz_ptr);

        std::cout << "    Schema Version    : " << schema_version << "\n"
                     "    Data Version      : " << data_version << "\n"
                     "    BaseData Version  : " << base_version << "\n"
                     "    Created on        : " << time << " (UTC)\n"
                     "    Created at        : " << host << "\n"
                     "    Created by        : " << user << "\n"
	             "    Description       : " << row["DESCRIPTION"].data<std::string>() << std::endl;	  
      }
    }

    if(base_version == -1) {
      std::ostringstream text;
      text << "Cannot get data for schema " << schema_version << " and data " << data_version << " versions.";
      throw std::runtime_error( text.str() );
    }
  }


  oks::ral::get_schema(kernel, session, schema, schema_version, false, verbose_level);

  if(pattern) {
    VerboseMsg2("Compare OKS classes with classes from schema " << schema_version);
    for(OksClass::Map::const_iterator class_it = pattern->begin(); class_it != pattern->end(); ++class_it) {
      if(OksClass * c = kernel.find_class(class_it->second->get_name())) {
        if(c->compare_without_methods(*class_it->second) == false) {
          VerboseMsg2( " * class \'" << class_it->second->get_name() << "\' stored in file \'" << class_it->second->get_file()->get_full_file_name()
                    << "\' differs from one stored in database (schema version " << schema_version << "); exiting get_data() ...");
          return 0;
        }
      }
      else {
        VerboseMsg2(" * class \'" << class_it->second->get_name() << "\' from \'" << class_it->second->get_file()->get_full_file_name() << "\' is not found in schema version " << schema_version);
      }
    }
  }

  coral::AttributeList BindList;
  BindList.extend<int>( "sv" );
  BindList.extend<int>( "bv" );
  BindList["sv"].data<int>() = schema_version;
  BindList["bv"].data<int>() = base_version;

  coral::AttributeList BindList4;
  BindList4.extend<int>( "sv" );
  BindList4.extend<int>( "bv" );
  BindList4.extend<int>( "dv" );
  BindList4.extend<short>( "st" );
  BindList4["sv"].data<int>() = schema_version;
  BindList4["bv"].data<int>() = base_version;
  BindList4["dv"].data<int>() = data_version;
  BindList4["st"].data<short>() = 1;

  coral::AttributeList BindList5;
  BindList5.extend<int>( "sv" );
  BindList5.extend<int>( "bv" );
  BindList5.extend<int>( "dv" );
  BindList5["sv"].data<int>() = schema_version;
  BindList5["bv"].data<int>() = base_version;
  BindList5["dv"].data<int>() = data_version;

  
    // keep info about read rows
  
  unsigned long readOksObjectRows = 0;
  unsigned long readOksDataRelRows = 0;
  unsigned long readOksDataValRows = 0;


    // keep objects to be removed from base data
    // NOTE: is not necessary, when RAL will support SET OPERATORS
    // select CLASSNAME,ID from OksObject where ClassName='InterfaceMap' and SchemaVersion=4 and (DataVersion=1 or (DataVersion=2 and State=1))
    // minus select CLASSNAME,ID from OksObject  where (ClassName='InterfaceMap'and SchemaVersion=4 and DataVersion=2 and State=0);

  std::map<std::string, std::set<std::string> > removed;
  std::map<std::string, std::set<std::string> > created;

  if(base_version != data_version) {
    VerboseMsg2("Detecting oks objects removed or created relative to base configuration...");
    coral::ITable& table = session->schema(schema).tableHandle( "OKSOBJECT" );

    coral::AttributeList BindList3;
    BindList3.extend<int>( "sv" );
    BindList3.extend<int>( "dv" );
    BindList3.extend<short>( "st" );
    BindList3["sv"].data<int>() = schema_version;
    BindList3["dv"].data<int>() = data_version;
    BindList3["st"].data<short>() = 2;

    std::unique_ptr< coral::IQuery > query( table.newQuery() );
    query->setRowCacheSize( 1000 );
    query->addToOutputList( "CLASSNAME");
    query->addToOutputList( "ID");
    query->addToOutputList( "STATE");
    query->setCondition( "SCHEMAVERSION = :sv AND DATAVERSION = :dv AND NOT STATE = :st", BindList3 );

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      readOksObjectRows++;
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      std::string cname = iColumn->data<std::string>(); ++iColumn;
      std::string objid = iColumn->data<std::string>(); ++iColumn;
      short state = iColumn->data<short>();
      if(state == 0) {
	removed[cname].insert(objid);
	VerboseMsg2(" * mark object \'" << objid << "\' as removed in class \'" << cname << '\'');
      }
      else if(state == 1) {
	created[cname].insert(objid);
	VerboseMsg2(" * mark object \'" << objid << "\' as created in class \'" << cname << '\'');
      }
    }
  }


    // keep list of updated object's attributes and relationships
    // if found in the base configuration, then skipped

  std::map<std::string, std::map<std::string, std::set<std::string> > > updated;

  if(base_version != data_version) {
    VerboseMsg3("Detecting updated attributes and relationships (comparing with base configuration)...");

    const char * tables[] = {"OKSDATAREL", "OKSDATAVAL"};
    unsigned long * counters[] = {&readOksDataRelRows, &readOksDataValRows};

    for(size_t idx = 0; idx < sizeof(tables) / sizeof(const char *); ++idx) {
      coral::ITable& table = session->schema(schema).tableHandle( tables[idx] );

      std::unique_ptr< coral::IQuery > query( table.newQuery() );

      query->setRowCacheSize( 1000 );

      coral::AttributeList BindList2;
      BindList2.extend<int>( "sv" );
      BindList2.extend<int>( "dv" );
      BindList2["sv"].data<int>() = schema_version;
      BindList2["dv"].data<int>() = data_version;

      query->setCondition( "SCHEMAVERSION = :sv AND DATAVERSION = :dv", BindList2 );

      query->addToOutputList( "CLASSNAME");
      query->addToOutputList( "OBJECTID");
      query->addToOutputList( "NAME");

      coral::ICursor& cursor = query->execute();

      while( cursor.next() ) {
        (*counters[idx])++;
	coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
	std::string cname = iColumn->data<std::string>(); ++iColumn;
	std::string objid = iColumn->data<std::string>(); ++iColumn;
	std::string aname = iColumn->data<std::string>();

	if(!is_defined(cname, objid, created)) {
	  updated[cname][objid].insert(iColumn->data<std::string>());
	  VerboseMsg3(" * mark \'" << iColumn->data<std::string>() << "\' as updated value of \'" << objid << '@' << cname << "\'");
	}
      }
    }
  }

  static bool use_order = ::getenv("OKS_RAL_ORDER_QUERY_RESULTS");
  static int objects_query_fetch_size = 0;

  if(objects_query_fetch_size == 0) {
    if(const char * fetch_size_val = ::getenv("OKS_RAL_OBJECTS_QUERY_FETCH_SIZE")) {
      objects_query_fetch_size = static_cast<int>(strtoul(fetch_size_val, 0, 0));
    }

    if(objects_query_fetch_size == 0) {
      objects_query_fetch_size = 25000;
    }

    VerboseMsg2("Set query parameters...");
    VerboseMsg2(" * use ordering of query results: " << (use_order ? "true" : "false"));
    VerboseMsg2(" * use objects query fetch size: " << objects_query_fetch_size);
  }


  struct timeval tv1, tv2;

    // read objects

  {
    VerboseMsg2("Get oks objects...");

    coral::ITable& table = session->schema(schema).tableHandle( "OKSOBJECT" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );
    query->setRowCacheSize( objects_query_fetch_size );
    query->addToOutputList( "/*+ INDEX_RS_ASC( OKSOBJECT(SCHEMAVERSION, DATAVERSION) ) */ CLASSNAME");
    query->addToOutputList( "ID");

    if(base_version == data_version) {
      query->setCondition( "SCHEMAVERSION = :sv AND DATAVERSION = :bv", BindList );
    }
    else {
      query->setCondition( "SCHEMAVERSION = :sv AND (DATAVERSION = :bv OR (DATAVERSION = :dv AND STATE = :st)) AND (CREATIONVERSION <= :dv)", BindList4 );
    }

    gettimeofday(&tv1, 0);

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      readOksObjectRows++;
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      std::string cname = iColumn->data<std::string>();
      if(OksClass * c  = kernel.find_class(cname)) {
	++iColumn;

	if(base_version != data_version) {
	  if(is_defined(cname, iColumn->data<std::string>(), removed)) {
	    VerboseMsg3(" * skip removed object " << iColumn->data<std::string>() << '@' << cname << " defined in the base configuration");
	    continue;
	  }
	}

	OksObject * o = new OksObject(c, iColumn->data<std::string>().c_str(), true);

	VerboseMsg3(" * got object " << o);
      }
      else {
	throw_cannot_find_class(cname);
      }
    }

    gettimeofday(&tv2, 0);

    VerboseMsg2(" * got objects in : " << (tv2.tv_sec-tv1.tv_sec)+(tv2.tv_usec-tv1.tv_usec)/1000000.0 << " seconds");

  }


  {
    VerboseMsg2("Get relationships between oks objects...");

    coral::ITable& table = session->schema(schema).tableHandle( "OKSDATAREL" );

    std::unique_ptr< coral::IQuery > query( table.newQuery() );

    query->setRowCacheSize( objects_query_fetch_size );

    if(base_version == data_version) {
      query->setCondition( "SCHEMAVERSION = :sv AND DATAVERSION = :bv", BindList );
    }
    else {
      query->setCondition( "SCHEMAVERSION = :sv AND (DATAVERSION = :bv OR DATAVERSION = :dv) AND (CREATIONVERSION <= :dv)", BindList5 );
    }

    if(use_order) {
      query->addToOrderList( "CLASSNAME" );
      query->addToOrderList( "OBJECTID" );
      query->addToOrderList( "NAME" );
      query->addToOrderList( "PARENTPOSITION" );
    }

    query->addToOutputList( "/*+ INDEX_RS_ASC( OKSDATAREL(SCHEMAVERSION, DATAVERSION) ) */ CLASSNAME");

    if(base_version != data_version) {
      query->addToOutputList( "DATAVERSION");
    }

    query->addToOutputList( "OBJECTID");
    query->addToOutputList( "NAME");
    query->addToOutputList( "PARENTPOSITION");
    query->addToOutputList( "VALUECLASSNAME");
    query->addToOutputList( "VALUEOBJECTID");

    gettimeofday(&tv1, 0);

    coral::ICursor& cursor = query->execute();

    while( cursor.next() ) {
      readOksDataRelRows++;
      coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
      std::string cname = iColumn->data<std::string>();
      if(OksClass * c  = kernel.find_class(cname)) {
	int reldv = -1;
        ++iColumn;

	if(base_version != data_version) {
	  reldv = iColumn->data<int>(); ++iColumn;
	  if(is_defined(cname, iColumn->data<std::string>(), removed)) {
	    VerboseMsg3(" * skip relationships of removed object " << iColumn->data<std::string>() << '@' << cname);
	    continue;
	  }
	}

	if(OksObject * o = c->get_object(iColumn->data<std::string>())) {
	  ++iColumn;

	  std::string relname = iColumn->data<std::string>();

	  if(reldv != data_version && is_defined(cname, o->GetId(), relname, updated)) {
	    VerboseMsg3(" * skip base data values of relationship " << relname << " of updated object " << o);
	    continue;
	  }

	  if(OksRelationship * r = c->find_relationship(relname)) {
	    ++iColumn;
	    int pos = iColumn->data<int>();

            VerboseMsg5("   + set_rel " << o << "[" << relname << "][" << pos << ']');

	    ++iColumn;
	    std::string val_cname = iColumn->data<std::string>(); ++iColumn;
            if(r->get_high_cardinality_constraint() == OksRelationship::Many) {
              OksData * d(o->GetRelationshipValue(r->get_name()));

              if(pos == 0) {
                d->Set(new OksData::List());
                continue;
              }
              else if(d->data.LIST->size() < static_cast<size_t>(pos)) {
                OksData * at = 0;
                while(d->data.LIST->size() < static_cast<size_t>(pos)) {
                  at = new OksData((OksObject *)0);
                  d->data.LIST->push_back(at);
                }
                d = at;
              }
              else {
                OksData::List::iterator i = d->data.LIST->begin();
                while(pos-- > 1) ++i;
                d = *i;
              }

	      if(OksClass * c2 = kernel.find_class(val_cname)) {
		if(OksObject * o2 = c2->get_object(iColumn->data<std::string>())) {
		  d->Set(o2);
                }
              }
	    }
	    else {
              try {
	        o->SetRelationshipValue(r->get_name(), val_cname, iColumn->data<std::string>());
              }
              catch(oks::exception& ex) {
                std::ostringstream s;
                s << "Internal error: failed to set relationship value: " << ex.what();
                throw std::runtime_error( s.str() );
              }
	    }
	  }
	  else {
	    throw_cannot_find_object_param("relationship", iColumn->data<std::string>(), o);
	  }
	}
	else {
	  throw_cannot_find_object(cname, iColumn->data<std::string>());
	}
      }
      else {
	throw_cannot_find_class(cname);
      }
    }

      // clean bad multi-value relationship values (e.g. happens, when parent position values are sparsed)

    for(OksClass::Map::const_iterator i = kernel.classes().begin(); i != kernel.classes().end(); ++i) {
      if(const std::list<OksRelationship *> * rels = i->second->all_relationships()) {
        if(size_t num_of_rels = rels->size()) {
          if(const OksObject::Map * objects = i->second->objects()) {
            size_t num_of_attrs = i->second->number_of_all_attributes();
            size_t full_size = num_of_rels + num_of_attrs;
            for(OksObject::Map::const_iterator j = objects->begin(); j != objects->end(); ++j) {
              std::list<OksRelationship *>::const_iterator r = rels->begin();
              for(size_t offset = num_of_attrs; offset < full_size; ++offset, ++r) {
                if((*r)->get_high_cardinality_constraint() == OksRelationship::Many) {
                  OksDataInfo odi(offset, *r);
                  OksData *d(j->second->GetRelationshipValue(&odi));
                  if(d->data.LIST) {
                    for(OksData::List::iterator i2 = d->data.LIST->begin(); i2 != d->data.LIST->end(); ) {
                      if((*i2)->type == OksData::object_type && (*i2)->data.OBJECT == 0) {
                        OksData * d2 = (*i2);
                        i2 = d->data.LIST->erase(i2);
                        VerboseMsg2(" * remove (null) value from value of relationship " << (*r)->get_name() << " of object " << j->second);
                        delete d2;
                      }
                      else {
                        ++i2;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    gettimeofday(&tv2, 0);

    VerboseMsg2(" * got relationship values in : " << (tv2.tv_sec-tv1.tv_sec)+(tv2.tv_usec-tv1.tv_usec)/1000000.0 << " seconds");

  }

  VerboseMsg2("Get attributes of oks objects...");

  size_t value_var_pos = 6;

  coral::ITable& table = session->schema(schema).tableHandle( "OKSDATAVAL" );

  std::unique_ptr< coral::IQuery > query( table.newQuery() );

  query->setRowCacheSize( objects_query_fetch_size );


  if(base_version == data_version) {
    query->setCondition( "SCHEMAVERSION = :sv AND DATAVERSION = :bv", BindList );
  }
  else {
    query->setCondition( "SCHEMAVERSION = :sv AND (DATAVERSION = :bv OR DATAVERSION = :dv) AND (CREATIONVERSION <= :dv)", BindList5 );
  }

  if(use_order) {
    query->addToOrderList( "CLASSNAME" );
    query->addToOrderList( "OBJECTID" );
    query->addToOrderList( "NAME" );
    query->addToOrderList( "PARENTPOSITION" );
  }

  query->addToOutputList( "/*+ INDEX_RS_ASC(OKSDATAVAL(SCHEMAVERSION, DATAVERSION)) */ CLASSNAME");

  if(base_version != data_version) {
    query->addToOutputList( "DATAVERSION");
    value_var_pos = 7;
  }

  query->addToOutputList( "OBJECTID");
  query->addToOutputList( "NAME");
  query->addToOutputList( "PARENTPOSITION");
  query->addToOutputList( "INTEGERVALUE");
  query->addToOutputList( "NUMBERVALUE");
  query->addToOutputList( "STRINGVALUE");

  static const char * int64_coral_name = ((sizeof(long) == sizeof(int64_t)) ? "long" : "long long");
  query->defineOutputType( "INTEGERVALUE", int64_coral_name);  // i.e. int64_t

  query->defineOutputType( "NUMBERVALUE", "double");

  gettimeofday(&tv1, 0);

  coral::ICursor& cursor = query->execute();

  while( cursor.next() ) {
    readOksDataValRows++;
    coral::AttributeList::const_iterator iColumn = cursor.currentRow().begin();
    std::string cname = iColumn->data<std::string>();
    if(OksClass * c  = kernel.find_class(cname)) {
      int attrdv = -1;
      ++iColumn;

      if(base_version != data_version) {
	attrdv = iColumn->data<int>(); ++iColumn;
	if(is_defined(cname, iColumn->data<std::string>(), removed)) {
	  VerboseMsg3(" * skip attributes of removed object " << iColumn->data<std::string>() << '@' << cname);
	  continue;
	}
      }

      if(OksObject * o = c->get_object(iColumn->data<std::string>())) {
	++iColumn;

	if(attrdv != data_version && is_defined(cname, o->GetId(), iColumn->data<std::string>(), updated)) {
	  VerboseMsg3(" * skip base data values of attribute " << iColumn->data<std::string>() << " of updated object " << o);
	  continue;
	}

	if(OksAttribute * a = c->find_attribute(iColumn->data<std::string>())) {
	  ++iColumn;
	  int pos = iColumn->data<int>();
	  ++iColumn;
	  set_value(*o, *a, iColumn, pos, cursor.currentRow()[value_var_pos].isNull(), verbose_level);
	}
	else {
	  throw_cannot_find_object_param("attribute", iColumn->data<std::string>(), o);
	}
      }
      else {
	throw_cannot_find_object(cname, iColumn->data<std::string>());
      }
    }
    else {
      throw_cannot_find_class(cname);
    }
  }
  
  gettimeofday(&tv2, 0);

  VerboseMsg2(" * got attribute values in : " << (tv2.tv_sec-tv1.tv_sec)+(tv2.tv_usec-tv1.tv_usec)/1000000.0 << " seconds");

  VerboseMsg2("Number of read rows by tables:\n"
              "  oks objects         : " << readOksObjectRows << "\n"
              "  relationship values : " << readOksDataRelRows << "\n"
	      "  attribute values    : " << readOksDataValRows );

  return (readOksObjectRows + readOksDataRelRows + readOksDataValRows);

}

////////////////////////////////////////////////////////////////////////////////

void tag_data(
            coral::ISessionProxy * session,
            const std::string& schema,
            int schema_version,
            int data_version,
            const std::string& tag,
            int verbose_level)
{
  VerboseMsg1("Tag schema version: \'" << schema_version << "\' and data version: \'" << data_version << "\' with tag \'" << tag << "\'...");

  coral::AttributeList data;
  coral::ITable& table = session->schema(schema).tableHandle( "OKSTAG" );
  table.dataEditor().rowBuffer( data );

  data["SCHEMAVERSION"].data<int>() = schema_version;
  data["DATAVERSION"].data<int>() = data_version;
  data["NAME"].data<std::string>() = tag;
  set_time_host_user(data);

  try {
    table.dataEditor().insertRow( data );
  }
  catch ( coral::SchemaException& e ) {
    throw_schema_exception(e, "OksTag");
  }
}

////////////////////////////////////////////////////////////////////////////////

void create_archive_record(
            coral::ISessionProxy * session,
            const std::string& schema,
            int schema_version,
            int data_version,
            const std::string& partition_name,
            int run_number,
            int verbose_level)
{
  VerboseMsg1(
    "Put record schema version: " << schema_version << " and data version: " << data_version <<
    " been used in partition \'" << partition_name << "\' for run number " << run_number << " ...");

  coral::AttributeList data;
  coral::ITable& table = session->schema(schema).tableHandle( "OKSARCHIVE" );
  table.dataEditor().rowBuffer( data );

  data["SCHEMAVERSION"].data<int>() = schema_version;
  data["DATAVERSION"].data<int>() = data_version;
  data["PARTITION"].data<std::string>() = partition_name;
  data["RUNNUMBER"].data<int>() = run_number;
  set_time_host_user(data);

  try {
    table.dataEditor().insertRow( data );
  }
  catch ( coral::SchemaException& e ) {
    throw_schema_exception(e, "OksArchive");
  }
}

////////////////////////////////////////////////////////////////////////////////

} }
