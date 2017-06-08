#ifndef __SYS_SERVICE_METHODS_H__
#define __SYS_SERVICE_METHODS_H__

#include <string>

namespace utils
{

namespace sys
{

namespace service
{

/// \brief Start daemon
void start_service( const std::string& service );

/// \brief Stop daemon
void stop_service( const std::string& service );

/// \brief Restart daemon
void restart_service( const std::string& service );

/// \brief Reload service conf
void reload_service( const std::string& service );

/// \brief Check if service is running
bool service_is_running( const std::string& service );

}

}

}


#endif
