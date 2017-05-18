#include "../sys_gpio_methods.h"

#include <thread>
#include <fcntl.h>
#include <unistd.h>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "../aux_methods.h"
#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

namespace gpio
{

namespace details
{
  gpio_direction str_to_direction( const std::string& direction_str )
  {
    gpio_direction direction;

    if( direction_str == "in" )
    {
      direction = gpio_direction::in;
    }
    else if( direction_str == "out" )
    {
      direction = gpio_direction::out;
    }
    else
    {
      throw std::invalid_argument{ "Invalid direction string" };
    }

    return direction;
  }

  std::string direction_to_str( const gpio_direction& direction )
  {
    return ( direction == gpio_direction::in )? "in" : "out";
  }
}

void enable_gpio_line( uint32_t line, const gpio_direction& direction )
{
  if( !gpio_line_enabled( line ) )
  {
    int gpio_enabler{ open( "/sys/class/gpio/export", O_WRONLY ) };
    if( gpio_enabler != -1 )
    {
      std::string line_str{ std::to_string( line ) };
      ssize_t res{ write( gpio_enabler, line_str.c_str(), line_str.length() ) };
      close( gpio_enabler );

      if( res < 0 || ( size_t )res != line_str.length() )
      {
        throw std::runtime_error{ "Could not write to export file" };
      }
    }
    else
    {
      throw std::runtime_error{ "Could not open export file" };
    }

    std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
  }

  set_gpio_line_direction( line, direction );
}

void disable_gpio_line( uint32_t line )
{
  if( !gpio_line_enabled( line ) )
  {
    return;
  }

  int gpio_line_file{ open( "/sys/class/gpio/unexport", O_WRONLY ) };
  if( gpio_line_file != -1 )
  {
    std::string line_str{ std::to_string( line ) };
    ssize_t res{ write( gpio_line_file, line_str.c_str(), line_str.size() ) };
    close( gpio_line_file );

    if( res < 0 || ( size_t )res != line_str.length() )
    {
      throw std::runtime_error{ "Could not write to unexport file" };
    }

    std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
  }
  else
  {
    throw std::runtime_error{ "Could not open unexport file" };
  }
}

void set_gpio_line_direction( uint32_t line, const gpio_direction& direction )
{
  if( !gpio_line_enabled( line ) )
  {
    throw std::runtime_error{ "Line not opened" };
  }

  std::string direction_path{ boost::str( boost::format{ "/sys/class/gpio/gpio%u/direction" } % line ) };

  int gpio_line_mode{ open( direction_path.c_str(), O_WRONLY ) };
  if( gpio_line_mode != -1 )
  {
    std::string direction_str{ details::direction_to_str( direction ) };
    ssize_t res{ write( gpio_line_mode, direction_str.c_str(), direction_str.length() ) };
    close( gpio_line_mode );

    if( res < 0 || ( size_t )res != direction_str.length() )
    {
      throw std::runtime_error{ "Could not write to line direction file" };
    }
  }
  else
  {
    throw std::invalid_argument{ "Could not open line direction file" };
  }
}

gpio_direction get_gpio_line_direction( uint32_t line )
{
  if( !gpio_line_enabled( line ) )
  {
    throw std::runtime_error{ "Line not opened" };
  }

  std::string direction_path{ boost::str( boost::format{ "/sys/class/gpio/gpio%u/direction" } % line ) };
  std::string direction_str{ utils::aux::read_file( direction_path ) };
  boost::trim_if( direction_str, boost::is_any_of( "\n" ) );
  return details::str_to_direction( direction_str );
}

bool gpio_line_enabled( uint32_t line )
{
  return boost::filesystem::exists( boost::str( boost::format{ "/sys/class/gpio/gpio%u" } % line ) );
}

void set_gpio_line_status( uint32_t line, int8_t status )
{
    if( !gpio_line_enabled( line ) )
    {
      throw std::invalid_argument{ "Line is disabled" };
    }

    if( get_gpio_line_direction( line ) != gpio_direction::out )
    {
      throw std::invalid_argument{ "Incorrect line direction" };
    }

    std::string path{ boost::str( boost::format{ "/sys/class/gpio/gpio%u/value" } % line ) };

    int gpio_line_file{ open( path.c_str(), O_WRONLY ) };
    if( gpio_line_file != -1 )
    {
        auto str_status = boost::lexical_cast< std::string >( ( uint32_t )status );
        ssize_t res{ write( gpio_line_file, str_status.c_str(), str_status.length() ) };
        close( gpio_line_file );

        if( res < 0 || ( size_t )res != str_status.length() )
        {
          throw std::runtime_error{ "Could not write to gpio line file" };
        }

        std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
    }
    else
    {
      throw std::runtime_error{ "Could not open gpio line file" };
    }
}

int8_t get_gpio_line_status( uint32_t line )
{
  if( !gpio_line_enabled( line ) )
  {
    throw std::invalid_argument{ "Line is disabled" };
  }

  if( get_gpio_line_direction( line ) != gpio_direction::in )
  {
    throw std::invalid_argument{ "Incorrect line direction" };
  }

  int8_t line_status{ 0 };
  std::string path{ boost::str( boost::format{ "/sys/class/gpio/gpio%u/value" } % line ) };

  int gpio_line_file{ open( path.c_str(), O_RDONLY ) };
  if( gpio_line_file != -1 )
  {
    char val{ 0 };
    ssize_t res{ read( gpio_line_file, &val, sizeof( val ) ) };
    close( gpio_line_file );

    if( res != sizeof( val ) )
    {
      throw std::runtime_error{ "Could not read from gpio line file" };
    }

    line_status = ( uint8_t )boost::lexical_cast< uint32_t >( val );
  }
  else
  {
    throw std::runtime_error{ "Could not open gpio line file" };
  }

  return line_status;
}

}// gpio

}// sys

}// utils
