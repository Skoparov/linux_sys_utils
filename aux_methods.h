#ifndef __AUX_CLASS_METHODS_H__
#define __AUX_CLASS_METHODS_H__

#include <string>
#include <cxxabi.h>
#include <functional>

#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS

#define DEF_MAX_FILE_SIZE 5000000

namespace utils
{

namespace aux
{

/// \brief Get demangled class name
template< class T >
std::string get_class_name( const T& type ) noexcept
{
  int status;
  return abi::__cxa_demangle( typeid( type ).name(), 0, 0, &status );
}

/// \brief Recursively copy folder
void copy_folder( const std::string & src, const std::string & dst );

/// \brief Read file to string. If max_size_limit.first == false => unlimited size
std::string read_file( const std::string& path, bool binary = false,
                       const std::pair< bool, size_t > max_size_limit = { true, DEF_MAX_FILE_SIZE } );

}

}


#endif
