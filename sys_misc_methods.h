#ifndef __MISC_SYSTEM_METHODS_H__
#define __MISC_SYSTEM_METHODS_H__

#include <string>

namespace utils
{

namespace sys
{

/// \brief Returns number of cores
uint hardware_concurrency();

/// \brief Reboot system instantaneously
void restart_system();

}// sys

}// utils

#endif
