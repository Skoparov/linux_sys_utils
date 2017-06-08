#ifndef __MISC_SYSTEM_METHODS_H__
#define __MISC_SYSTEM_METHODS_H__

#include <string>

namespace utils
{

namespace sys
{

/// \brief Get arbitrary installed package version,
/// or empty string if it's not found or not installed
std::string get_package_version( const std::string& package_name );

/// \brief Returns number of cores
uint hardware_concurrency();

/// \brief Reboot system instantaneously
void restart_system();

}// sys

}// utils

#endif
