#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

#include <ers/ers.h>

#include <oks/tz.h>

oks::tz::DB::DB()
{
  const char * tz_spec_file = ::getenv("BOOST_DATE_TIME_TZ_SPEC");

  if(!tz_spec_file || !*tz_spec_file)
    {
      throw std::runtime_error ("cannot read value of BOOST_DATE_TIME_TZ_SPEC environment variable");
    }

  ERS_DEBUG(1, "Boost time-zone specification file is \'" << tz_spec_file << '\'');

  try
    {
      m_tz_db.load_from_file(tz_spec_file);
    }
  catch(const std::exception& ex)
    {
      std::ostringstream text;
      text << "cannot read Boost time-zone specification file \"" << tz_spec_file << "\": " << ex.what();
      throw std::runtime_error (text.str().c_str());
    }
}

boost::local_time::time_zone_ptr
oks::tz::DB::get_tz_ptr(const std::string& region)
{
  boost::local_time::time_zone_ptr tz_ptr = m_tz_db.time_zone_from_region(region);

  if (!tz_ptr)
    {
      std::ostringstream text;
      text << "cannot find time-zone \'" << region << '\'';
      throw std::runtime_error(text.str().c_str());
    }

  return tz_ptr;
}

std::vector<std::string>
oks::tz::DB::get_regions()
{
  return m_tz_db.region_list();
}

boost::posix_time::ptime
oks::tz::str_2_posix_time(const std::string& in, boost::local_time::time_zone_ptr tz_ptr, const char * value)
{
  std::string s(in);
  std::replace(s.begin(), s.end(), 'T', ' ');

  boost::posix_time::ptime t;

  // convert string to time
  try
    {
      t = boost::posix_time::time_from_string(s);
    }
  catch (const std::exception& ex)
    {
      std::ostringstream text;
      text << "cannot parse " << value << " = \'" << in << "\': \"" << ex.what() << "\" (ISO 8601 format (YYYY-MM-DD HH:MM:SS) is expected).";
      throw std::runtime_error(text.str().c_str());
    }


  // convert local time to UTC, if the time zone was provided
  if (tz_ptr)
    {
      try
        {
          boost::local_time::local_date_time lt(t.date(), t.time_of_day(), tz_ptr, boost::local_time::local_date_time::EXCEPTION_ON_ERROR);
          ERS_DEBUG(1, "Build zone\'s time \'" << in << "\' => \'" << lt.to_string() << "\' using \'" << tz_ptr->to_posix_string() << '\'');
          t = lt.utc_time();
        }
      catch(std::exception& e)
        {
          std::ostringstream text;
          text << "cannot parse " << value << " = \'" << in << "\' in time zone \"" << tz_ptr->to_posix_string() << "\": \"" << e.what() << '\"' << std::endl;
          throw std::runtime_error(text.str().c_str());
        }
    }

  return t;
}

uint64_t
oks::tz::posix_time_2_to_ns(boost::posix_time::ptime t)
{
  static boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
  return ((t - epoch).total_nanoseconds());
}

std::string
oks::tz::posix_time_2_iso_string(boost::posix_time::ptime t)
{
  return boost::posix_time::to_iso_string(t);
}
