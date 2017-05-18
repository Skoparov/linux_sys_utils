#ifndef __FILE_SYSTEM_METHODS_H__
#define __FILE_SYSTEM_METHODS_H__

#include <string>

namespace utils
{

namespace sys
{

namespace file
{

/// \brief Moves file, or copies it and deletes the original
/// in case of cross-partition move
void move_file( const std::string& from, const std::string& to );

/// \brief Returns partition name of the path
std::string get_partition_by_path( const std::string& path );

/// \brief Checks if a device is mounted to the specified path
bool device_is_mounted_to_path( const std::string& path );

}// file

}// sys

}// utils

#endif
