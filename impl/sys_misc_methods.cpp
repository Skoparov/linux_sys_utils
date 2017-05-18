#include "../sys_misc_methods.h"

#include <thread>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <sys/reboot.h>

#include <boost/filesystem.hpp>

#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

uint hardware_concurrency()
{
  unsigned int result{ std::thread::hardware_concurrency() };
  if( !result )
  {
    result = sysconf( _SC_NPROCESSORS_ONLN );
  }

  return result;
}

void restart_system()
{
  sync();
  reboot( RB_AUTOBOOT );
}

// user

}// sys

}// utils
