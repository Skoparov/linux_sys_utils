#include "../sys_app_methods.h"

#include <array>
#include <unistd.h>
#include <linux/limits.h>
#include <boost/filesystem.hpp>

namespace utils
{

namespace sys
{

namespace app
{

std::string get_application_path()
{
  std::array< char, PATH_MAX > res;
  ssize_t cnt{ readlink( "/proc/self/exe", res.data(), res.size() ) };
  return std::string( res.data(), cnt > 0 ? cnt : 0 );
}

std::string get_application_dir()
{
  boost::filesystem::path app_path{ get_application_path() };
  return  app_path.parent_path().string();
}

}// app

}// sys

}// utils
