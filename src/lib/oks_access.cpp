#include <mutex>

#include <AccessManager/util/ErsIssues.h>
#include <AccessManager/client/RequestorInfo.h>
#include <AccessManager/client/ServerInterrogator.h>
#include <AccessManager/xacml/impl/DBResource.h>

#include <oks/access.h>

bool
oks::access::is_writable(const OksFile& file, const std::string& user)
{
  if (file.is_repository_file())
    {
      static std::mutex mutex;
      static bool inited(false);
      static bool am_is_on(false);
      static daq::am::ServerInterrogator * s_srv_inter(0);
      static std::map<std::string, daq::am::RequestorInfo *> s_request_info_by_user;
      static std::map<std::string, bool> s_is_user_admin;
      bool allowed = false;

      std::lock_guard<std::mutex> lock(mutex);

      if (!inited)
        {
          inited = true;
          const char * var = getenv("TDAQ_AM_AUTHORIZATION");
          if (var && !strcmp(var, "on"))
            {
              am_is_on = true;
              s_srv_inter = new daq::am::ServerInterrogator();
              ERS_DEBUG(1, "AccessManger is ON");
            }
          else
            {
              ERS_DEBUG(1, "AccessManger is OFF");
            }
        }

      if(am_is_on)
        {
          daq::am::RequestorInfo *& access_subject(s_request_info_by_user[user]);
          bool user_is_db_admin;

          if (access_subject == 0)
            {
              access_subject = new daq::am::RequestorInfo(user, OksKernel::get_host_name());

              std::unique_ptr< const daq::am::DBResource > db_admin_res( daq::am::DBResource::getInstanceForAdminOperations() );

              try
                {
                  user_is_db_admin = s_srv_inter->isAuthorizationGranted(*db_admin_res, *access_subject);
                  s_is_user_admin[user] = user_is_db_admin;
                  ERS_DEBUG(1, "daq::am::ServerInterrogator::isAuthorizationGranted(" << user << ", \'AdminOperations\') returns " << user_is_db_admin);
                }
              catch (daq::am::Exception &ex)
                {
                  s_is_user_admin[user] = false;
                  std::ostringstream text;
                  text << "daq::am::ServerInterrogator::isAuthorizationGranted(" << user << ", \'AdminOperations\') failed:\n" << ex.what();
                  throw std::runtime_error(text.str().c_str());
                }
            }
          else
            {
              user_is_db_admin = s_is_user_admin[user];
            }

          if (user_is_db_admin) return true;

          std::unique_ptr< const daq::am::DBResource > db_file_res( daq::am::DBResource::getInstanceForFileOperations(file.get_repository_name(), daq::am::DBResource::ACTION_ID_UPDATE_FILE) );

          try
            {
              allowed = s_srv_inter->isAuthorizationGranted(*db_file_res, *access_subject);
            }
          catch (daq::am::Exception &ex)
            {
              std::ostringstream text;
              text << "daq::am::ServerInterrogator::isAuthorizationGranted(" << user << ", \'" << file.get_repository_name() << "\', UPDATE_FILE) failed:\n" << ex.what();
              throw std::runtime_error(text.str().c_str());
            }

          ERS_DEBUG(1, "daq::am::ServerInterrogator::isAuthorizationGranted(" << user << ", \'" << file.get_repository_name() << "\', UPDATE_FILE) returns " << allowed);
        }
      else
        {
          return true; // FIXME 2020-04-16: see https://its.cern.ch/jira/browse/ADTCC-227
        }

      return allowed;
    }
  else
    {
      std::string text("oks::access::is_writable(\"");
      text += file.get_short_file_name();
      text += "\", ...) failed: the file is not stored on OKS server repository";
      throw std::runtime_error(text.c_str());
    }
}
