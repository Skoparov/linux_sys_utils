#include "../sys_time_methods.h"

#include <ctime>
#include <time.h>
#include <cstdio>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "../aux_methods.h"
#include "execute_sys_command.h"

#define TIMEZONE_FILE "/etc/timezone"

namespace utils
{

namespace sys
{

namespace time
{

std::string get_time( const std::string& format )
{
    time_t now{ ::time( 0 ) };
    tm  tstruct = *localtime( &now );

    std::array< char, 80 > buf;
    strftime( buf.data(), buf.size(), format.c_str(), &tstruct );

    return buf.data();
}

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

std::string get_ntp_time_from_server(  const std::string &server, bool local, const std::string& format, uint32_t attempts )
{
  int sock{ socket( PF_INET, SOCK_DGRAM, 0 ) };
  if( sock == -1 )
  {
      throw std::runtime_error{ std::string{ "Failed to open socket: " } + std::strerror( errno ) };
  }

  BOOST_SCOPE_EXIT( sock ){ close( sock ); } BOOST_SCOPE_EXIT_END

  struct timeval tv{ 0, 200000 };// timeout = 200ms

  if( ( setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) ) != 0 ) ||
      ( setsockopt( sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof( tv ) ) != 0 ) )
  {
    throw std::runtime_error{ std::string{ "setsockopt failed: " } + std::strerror( errno ) };
  }

  hostent* he{ gethostbyname( server.c_str() ) };
  if( !he )
  {
    throw std::runtime_error{ "Failed to resolve host name: " + server };
  }

  static const int portnum{ 123 };

  struct sockaddr_in server_addr;
  memset( &server_addr, 0, sizeof( sockaddr_in ) );
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr = *reinterpret_cast< in_addr* >( he->h_addr_list[ 0 ] );
  server_addr.sin_port = htons( portnum );

  static const size_t maxlen{ 1024 };
  std::vector< unsigned long >  buf( maxlen );
  std::array< unsigned char, 48 > msg{ 010, 0, 0, 0, 0, 0, 0, 0, 0 }; // the packet

  int res{ -1 };

  for( uint32_t attempt{ 0 }; attempt < attempts && ( res < 0 ); ++attempt )
  {
    res = sendto( sock, msg.data(), msg.size(), 0, reinterpret_cast< sockaddr* >( &server_addr ), sizeof( server_addr ) );
    if( res < 0 )
    {
      continue;
    }

    struct sockaddr saddr;
    socklen_t saddr_l = sizeof( saddr );

    res = recvfrom( sock, buf.data(), 48, 0, &saddr, &saddr_l );
    if( res < 0 )
    {
      continue;
    }
  }

  if( res < 0 )
  {
    std::stringstream s;
    s << "Failed to receive time from " << server << " after " << attempts << " attempts";
    throw std::runtime_error{ s.str() };
  }

  long tmit{ ntohl( static_cast< time_t >( buf.at( 4 ) ) ) };
  tmit -= 2208988800U;

  boost::posix_time::ptime now = local?
                boost::posix_time::ptime_from_tm( *localtime( &tmit ) ) :
                boost::posix_time::ptime_from_tm( *gmtime( &tmit ) );

  static std::locale loc( std::cout.getloc(),
                            new boost::posix_time::time_facet{ format.c_str() } );

  std::basic_stringstream<char> wss;
  wss.imbue( loc );
  wss << now;

  return wss.str();
}

}// time

}// sys

}// utils
