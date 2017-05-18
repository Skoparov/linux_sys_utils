#ifndef __SYS_CRON_METHODS_H__
#define __SYS_CRON_METHODS_H__

#include <string>

namespace utils
{

namespace sys
{

namespace cron
{

/// \brief Add cron script for the user
void add_cron_script( const std::string& user, const std::string& script_path );

/// \brief Remove all cron scripts for the user
void remove_all_cron_scripts( const std::string& user );

}

}

}


#endif
