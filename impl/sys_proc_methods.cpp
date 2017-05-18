#include "../sys_proc_methods.h"
#define BOOST_TEST_MAIN
#define BOOST_TEST_STATIC_LINK

#include <pwd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "../aux_methods.h"
#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

namespace proc
{

std::string get_file_owner( const std::string& path )
{
  if( !boost::filesystem::exists( path ) )
  {
    throw std::invalid_argument{ "Path is invalid" };
  }

  struct stat info;
  if( stat( path.c_str(), &info ) != 0 )
  {
    throw std::runtime_error{ "Failed to get stat info" };
  }

  passwd* pw{ getpwuid( info.st_uid ) };
  return pw->pw_name;
}

std::vector< pid_t > get_pids( const std::string& proc_name_regex, const std::string& username )
{
  namespace bfs = boost::filesystem;
  std::vector< pid_t > pids;

  bfs::path p{ "/proc" };
  boost::format cmdline_format{ "%s/cmdline" };
  boost::regex pid_regex{ "[0-9]+" };
  boost::regex proc_regex{ proc_name_regex };

  for( bfs::directory_entry& entry : boost::make_iterator_range( bfs::directory_iterator( p ), {} ) )
  {
    std::string name{ entry.path().filename().string() };

    if( boost::regex_match( name, pid_regex ) && bfs::is_directory( entry ) )
    {
      std::string cmd_path{ boost::str( cmdline_format % entry.path().string() ) };
      std::string cmd_line{ aux::read_file( cmd_path ) };

      if( !cmd_line.empty() )
      {
        size_t pos{ cmd_line.find( '\0' ) };
        if ( pos != std::string::npos )
        {
          cmd_line = cmd_line.substr( 0, pos );
        }

        std::vector< std::string > parts;
        boost::split( parts, cmd_line, boost::is_any_of( "/" ) );

        if( parts.empty() )
        {
          continue;
        }

        if( boost::regex_match( parts[ parts.size() - 1 ], proc_regex ) &&
            ( username.empty() || username == get_file_owner( entry.path().string() ) ) )
        {
          pids.emplace_back( std::stoi( name ) );
        }
      }
    }
  }

  return pids;
}

int kill_by_pid( pid_t pid, int sig )
{
  if( pid <= 0 )
  {
    throw std::invalid_argument{ "Pid should be positive" };
  }

  return ::kill( pid, sig );
}

std::map< pid_t, int > kill_by_procname( const std::string& procname_regex, int sig )
{
  std::vector< pid_t > pids{ get_pids( procname_regex ) };
  std::map< pid_t, int > errors;

  for( auto pid : pids )
  {
    int result{ kill_by_pid( pid, sig ) };
    if( result != 0 )
    {
      errors.emplace( pid, result );
    }
  }

  return errors;
}

std::map< pid_t, int > kill_by_user( const std::string& username, int sig )
{
  std::vector< pid_t > pids{ get_pids( ".*", username ) };
  std::map< pid_t, int > errors;

  for( auto pid : pids )
  {
    int result{ kill_by_pid( pid, sig ) };
    if( result != 0 )
    {
      errors.emplace( pid, result );
    }
  }

  return errors;
}

}// proc

}// sys

}// utils
