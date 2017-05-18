#include "execute_sys_command.h"

#include <memory>

#include <boost/algorithm/string.hpp>

#include <stdio.h>
#include <cstdio>

namespace utils
{

namespace sys
{

namespace details
{

std::string execute_sys_command( const std::string& command )
{
  if( command.empty() )
  {
    throw std::invalid_argument{ "Command is empty" };
  }

  std::unique_ptr< FILE, int( * )( FILE* ) > pipe{ popen( command.c_str(), "r" ), pclose };
  if( !pipe )
  {
    throw std::runtime_error{ "Failed to open pipe for command: " + command };
  }

  std::string output;
  std::array< char, 1024 > buffer;

  while( !feof( pipe.get() ) )
  {
    if ( fgets( buffer.data(), buffer.size(), pipe.get() ) )
    {
      output += buffer.data();
    }
  }

  boost::trim_if( output, boost::is_any_of( "\n" ) );

  return output;
}

}

}

}
