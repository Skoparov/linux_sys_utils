#include "../sys_misc_methods.h"

#include <thread>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <sys/reboot.h>

#include <boost/filesystem.hpp>

#include "../aux_methods.h"
#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

std::string get_package_version( const std::string& package_name )
{
    if( package_name.empty() )
    {
        throw std::invalid_argument{ "Invalid package name" };
    }

    std::string version;

    static const std::string dpkg_status_file{ "/var/lib/dpkg/status" };
    std::string file_data{ utils::aux::read_file( dpkg_status_file ) };

    size_t package_name_pos{ file_data.find( package_name ) };
    if( package_name_pos != std::string::npos )
    {
        auto get_param_value = [ & ]( const std::string& param )
        {
            size_t param_pos{ file_data.find( param, package_name_pos ) };
            if( param_pos != std::string::npos )
            {
                size_t line_end{ file_data.find( "\n", param_pos ) };
                return file_data.substr( param_pos + param.length(), line_end - ( param_pos + param.length() ) );
            }
        };

        std::string status{ get_param_value( "Status: " ) };
        if( status ==  "install ok installed" )
        {
            version = get_param_value( "Version: " );
        }
    }

    return version;
}

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
