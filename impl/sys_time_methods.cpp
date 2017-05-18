#include "../sys_time_methods.h"

#include <ctime>
#include <time.h>
#include <cstdio>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include "../aux_methods.h"
#include "execute_sys_command.h"

#define TIMEZONE_FILE "/etc/timezone"

namespace utils
{

namespace sys
{

namespace time
{

void set_sys_time( uint year, uint month, uint day, uint hour, uint min, uint sec )
{
  if( year < 1900 || month < 1 || month > 12 || day > 31 || hour > 24 || min > 60 || sec > 60 )
  {
    throw std::invalid_argument{ "invalid time settings" };
  }

  std::tm time;
  time.tm_sec = sec;
  time.tm_min = min;
  time.tm_hour = hour;
  time.tm_mday = day;
  time.tm_mon = month - 1;
  time.tm_year = year - 1900;

  time_t sys_time{ std::mktime( &time ) };
  int result{ ::stime( &sys_time ) };
  if( result != 0 )
  {
    throw std::runtime_error{ std::string{ "Failed to set system time: " } + strerror( result ) };
  }

  // Поменялся биос, требуется синхронизация часов с ним, иначе после перезагрузки откатывается
  details::execute_sys_command( "hwclock --systohc" );
}

std::string get_time_zone()
{
  std::string zone{ aux::read_file( TIMEZONE_FILE ) };
  boost::trim_if( zone, boost::is_any_of( "\n" ) );

  if( zone.empty() )
  {
    throw std::logic_error{ "Timezone file appears to be empty" };
  }

  return zone;
}

void set_time_zone( const std::string& timezone )
{
  if( timezone.empty() )
  {
    throw std::invalid_argument{ "Invalid timezone" };
  }

  std::string command{ std::string{ "timedatectl set-timezone " } + timezone };
  if( ::system( command.c_str() ) != 0 )
  {
    throw std::runtime_error{ "Could not set time zone" };
  }
}

std::vector<std::string> get_time_zones()
{
  std::vector< std::string > zones;

  std::string command_result{ details::execute_sys_command( "timedatectl list-timezones" ) };
  boost::split( zones, command_result, boost::is_any_of( "\n\r" ), boost::token_compress_on );
  zones.push_back( "Etc/UTC" );
  std::sort( zones.begin(), zones.end() );
  zones.erase( std::unique( zones.begin(), zones.end() ), zones.end() );

  return zones;
}

std::pair< std::string, double > get_time_zone_offset( const std::string& timezone )
{
  if( timezone.empty() )
  {
    throw std::invalid_argument{ "Invalid timezone" };
  }

  if( timezone == "Etc/UTC" )
  {
    return { "GMT+00.00", 0 };
  }

  boost::format offset_format{ "GMT%s" };

  std::string command{ "env TZ=" + timezone + "  date +%:z" };
  std::string offset_str{ details::execute_sys_command( command ) };

  boost::trim_if( offset_str, boost::is_any_of( "\n\r" ) );
  static const boost::regex r{ "[+-][0-9]{2}:[0-9]{2}" };
  if( !boost::regex_match( offset_str, r ) )
  {
    throw std::runtime_error{ "Invalid offset string" };
  }

  offset_str[ 3 ] = '.';
  return { boost::str( offset_format % offset_str ), std::stod( offset_str ) };
}

}// time

}// sys

}// utils
