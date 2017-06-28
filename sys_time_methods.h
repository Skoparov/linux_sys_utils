#ifndef __SYS_TIME_METHODS_H__
#define __SYS_TIME_METHODS_H__

#include <string>
#include <vector>

namespace utils
{

namespace sys
{

namespace time
{

/// \brief Get current system time with format used by strftime.
/// Default format is %Y.%m.%d %X
std::string get_time( const std::string& format = "%Y.%m.%d %X" );

/// \brief Set system time
void set_sys_time( uint year, uint month, uint day, uint hour, uint min, uint sec );

/// \brief Get current time zone
std::string get_time_zone();

/// \brief Set current time zone
void set_time_zone( const std::string& timezone );

/// \brief Get time zones from system
std::vector< std::string > get_time_zones();

/// \brief Get time zone offset as string and value
std::pair< std::string, double > get_time_zone_offset( const std::string& time_zone );

/// \brief Query ntp server
std::string get_ntp_time_from_server( const std::string &server,
                                     bool local = false,
                                     const std::string& format = "%Y.%m.%d %X",
                                     uint32_t attempts = 5 );

}

}

}


#endif
