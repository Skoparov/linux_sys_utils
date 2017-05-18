#include "../sys_user_methods.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <boost/filesystem.hpp>
#include <boost/iterator/filter_iterator.hpp>

#include "../sys_misc_methods.h"
#include "../aux_methods.h"
#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

namespace user
{

double get_cpu_load_by_user( const std::string& username )
{
  if( username.empty() )
  {
    throw std::invalid_argument{ "Invalid user" };
  }

  std::string command{ "top -b -n 1 -u " + username + " | awk 'NR>7 { sum += $9; } END { print sum; }'" };
  std::string output{ details::execute_sys_command( command ) };

  if( output.find( "Invalid user" ) != std::string::npos )
  {
    throw std::invalid_argument{ "Invalid user" };
  }

  double result{ 0.0 };

  try
  {
    if( !output.empty() )
    {
      result = std::stod( output ) / hardware_concurrency();
    }
  }
  catch( std::invalid_argument& )
  {
    throw std::runtime_error{ "Invalid output format : cast failed" };
  }

  return result;
}

namespace details
{

using func_type = std::function< void( const boost::filesystem::path& ) >;
using pred_type = std::function< bool( const boost::filesystem::path& ) >;

// User for both chmod and chown
void apply_action( const std::string& target,
                  const func_type& func,
                  target_type type,
                  const recursive& is_recursive )
{
  namespace bfs = boost::filesystem;

  if( target.empty() )
  {
    throw std::invalid_argument{ "Target is invalid" };
  }

  if( type != dir && type != file && type != symlink && type != all )
  {
    throw std::invalid_argument{ "Invalid type of targets" };
  }

  // create pred
  pred_type pred;
  if( type == all )
  {
    pred = []( const boost::filesystem::path& ){ return true; };
  }
  else
  {
    pred = [ type ]( const boost::filesystem::path& target )
    {
      return ( ( ( type & dir ) && boost::filesystem::is_directory( target ) )||
               ( ( type & file ) && boost::filesystem::is_regular_file( target ) )||
               ( ( type & symlink ) && boost::filesystem::is_symlink( target ) ) );
    };
  }

  if( pred( target ) )
  {
    func( target );
  }

  if( is_recursive == recursive::yes && bfs::is_directory( target ) )
  {
    std::for_each( boost::make_filter_iterator( pred, bfs::recursive_directory_iterator{ target }, bfs::recursive_directory_iterator{} ),
                   boost::make_filter_iterator( pred, bfs::recursive_directory_iterator{}, bfs::recursive_directory_iterator{} ),
                   func );
  }
}

}// details

void chmod( const std::string& target, int rights,
            target_type type,
            const recursive& is_recursive )
{
  if( rights < 00 || rights > 0777 )
  {
    throw std::invalid_argument{ "Invalid rights" };
  }

  details::func_type chmod_func =
  [ rights ]( const boost::filesystem::path& target )
  {
    if( ::chmod( target.string().c_str(), rights ) != 0 )
    {
      throw std::runtime_error{ std::string{ "Could not change permissions for file: " } + target.string() };
    }
  };

  details::apply_action( target, chmod_func, type, is_recursive );
}

void chown( const std::string& target,
            const std::pair< std::string, std::string >& user_group,
            target_type type,
            const recursive& is_recursive,
            const deref_symlinks& deref_sym_links)
{
  if( user_group.first.empty() && user_group.second.empty() )
  {
    throw std::invalid_argument{ "User and group can't be empty simultaneously" };
  }

  int uid{ -1 };
  int gid{ -1 };

  // user name
  if( !user_group.first.empty() )
  {
    passwd* user_info{ getpwnam( user_group.first.c_str() ) };
    if( !user_info )
    {
      throw std::invalid_argument{ "Invalid user" };
    }

    uid = user_info->pw_uid;
  }

  // group
  if( !user_group.second.empty() )
  {
    group* group_info{ getgrnam( user_group.second.c_str() ) };
    if( !group_info )
    {
      throw std::invalid_argument{ "Invalid group" };
    }

    gid = group_info->gr_gid;
  }

  auto func = deref_sym_links == deref_symlinks::yes? ::lchown : ::chown;

  details::func_type chown_func =
  [ gid, uid, func ]( const boost::filesystem::path& target )
  {
    if( func( target.string().c_str(), uid, gid ) != 0 )
    {
      throw std::runtime_error{ std::string{ "Could not change ownership for file: " } + target.string() };
    }
  };

  details::apply_action( target, chown_func, type, is_recursive );
}

}// user

}// sys

}// utils
