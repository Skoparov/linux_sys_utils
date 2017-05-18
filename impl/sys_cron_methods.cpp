#include "../sys_cron_methods.h"

#include <sstream>

#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

namespace cron
{

void add_cron_script( const std::string& user, const std::string& script_path )
{
  if( user.empty() )
  {
    throw std::invalid_argument{ "Invalid user name" };
  }

  std::stringstream ss;
  ss << "crontab -u " << user << " " << script_path;

  details::execute_sys_command( ss.str() );
}

void remove_all_cron_scripts( const std::string& user )
{
  if( user.empty() )
  {
    throw std::invalid_argument{ "Invalid user name" };
  }

  details::execute_sys_command( "crontab -r -u " + user );
}

}// cron

}// sys

}// utils
