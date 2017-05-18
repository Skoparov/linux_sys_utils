#include "../sys_file_methods.h"

#include <mntent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS

#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

namespace file
{

void move_file( const std::string& from, const std::string& to )
{
  if( !boost::filesystem::exists( from ) )
  {
    throw std::invalid_argument{ "Source does not exist" };
  }

  std::string from_part{ get_partition_by_path( from ) };
  std::string to_part{ boost::filesystem::is_directory( to )?
                       to : boost::filesystem::path{ to }.parent_path().string() };

  if( from_part == to_part ) // same partition
  {
    if( std::rename( from.c_str(), to.c_str() ) != 0 )
    {
      throw std::runtime_error{ std::string{ "Failed to move file: " } + std::strerror( errno ) };
    }
  }
  else // cross-partition
  {
    boost::filesystem::copy_file( from, to,
                                  boost::filesystem::copy_option::overwrite_if_exists );
    std::remove( from.c_str() );
  }
}

std::string get_partition_by_path( const std::string& path )
{
  if( path.empty() )
  {
    throw std::invalid_argument{ "Path is empty" };
  }

  struct stat s;
  if( stat( path.c_str(), &s ) != 0 )
  {
    throw std::runtime_error{ std::string{ "stat failed for path: " } + path + " : " + std::strerror( errno ) };
  }

  dev_t dev{ s.st_dev };

  std::unique_ptr< FILE, int( * )( FILE* ) > fp{ setmntent( "/proc/mounts", "r" ), endmntent };
  if( !fp )
  {
    throw std::runtime_error{ "setmntent failed on /proc/mounts" };
  }

  std::string partition_name;
  mntent mnt;
  std::array< char, 512 > arr;

  while( getmntent_r( fp.get(), &mnt, arr.data(), arr.size() ) )
  {
    if( stat( mnt.mnt_dir, &s ) == 0 &&
        s.st_dev == dev &&
        ( strcmp( mnt.mnt_fsname, "rootfs" ) != 0 ) )
    {
      partition_name = mnt.mnt_fsname;
      break;
    }
  }

  if( partition_name.empty() )
  {
    throw std::runtime_error{ "Failed to find partition" };
  }

  if( boost::filesystem::is_symlink( partition_name ) )
  {
    partition_name = boost::filesystem::canonical( partition_name ).string();
  }

  return partition_name;
}

bool device_is_mounted_to_path( const std::string& path )
{
  if( !boost::filesystem::exists( path ) )
  {
    throw std::invalid_argument{ "Path is invalid" };
  }

  std::unique_ptr< FILE, int( * )( FILE* ) > fp{ setmntent( "/proc/mounts", "r" ), endmntent };
  if( !fp )
  {
    throw std::runtime_error{ "setmntent failed on /proc/mounts" };
  }

  mntent mnt;
  std::array< char, 512 > arr;

  while( getmntent_r( fp.get(), &mnt, arr.data(), arr.size() ) )
  {
    if( strcmp( mnt.mnt_dir, path.c_str() ) == 0 )
    {
        return true;
    }
  }

  return false;
}


}// file

}// sys

}// utils
