#include "../sys_arch_methods.h"

#include <boost/filesystem.hpp>

#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

namespace arch
{

uintmax_t create_archive( const std::vector< std::string >& paths, const std::string& archive_path, const arch_type& type )
{
  namespace bfs = boost::filesystem;

  if( archive_path.empty() )
  {
    throw std::invalid_argument{ "Invalid archive path" };
  }

  if( paths.empty() )
  {
    throw std::invalid_argument{ "Cannot create empty archive" };
  }

  std::string command;
  if( type == arch_type::tar_gz )
  {
    command = "tar -czf ";
  }
  else if( type == arch_type::_7z )
  {
    command = "7zr a ";
  }

  command += archive_path + " ";

  for( const std::string& path : paths )
  {
    command += path + " ";
  }

  std::string out{ details::execute_sys_command( command ) };
  if( !bfs::exists( archive_path ) )
  {
    throw std::runtime_error{ "Failed to create archive:" + out };
  }

  return bfs::file_size( archive_path );
}

void extract_archive( const std::string& archive_path, const std::string& dest, const arch_type& type )
{
  namespace bfs = boost::filesystem;

  bfs::path arch_path{ archive_path };
  if( !bfs::is_regular_file( arch_path ) )
  {
    throw std::invalid_argument{ "Archive does not exist" };
  }

  if( !bfs::exists( dest ) && !bfs::create_directory( dest ) )
  {
    throw std::invalid_argument{ "Failed to create destination dir" };
  }

  std::string command;
  if( type == arch_type::tar_gz )
  {
    command = "tar -xf " + archive_path + " -C " + dest;
  }
  else if( type == arch_type::_7z )
  {
    command = "7zr x " + archive_path + " -o" + dest;
  }

  std::string out{ details::execute_sys_command( command ) };
  if( out.length() && type == arch_type::tar_gz )
  {
    throw std::runtime_error{ "Failed to extract archive: " + out };
  }
}

}// archive

}// sys

}// utils
