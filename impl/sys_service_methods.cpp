#include "../sys_service_methods.h"
#include "execute_sys_command.h"

namespace utils
{

namespace sys
{

namespace service
{

void start_service(const std::string &service)
{
  if( service.empty() )
  {
    throw std::invalid_argument{ "Invalid service name" };
  }

  details::execute_sys_command( "service " + service + " start" );
}

void stop_service( const std::string& service )
{
  if( service.empty() )
  {
    throw std::invalid_argument{ "Invalid service name" };
  }

  details::execute_sys_command( "service " + service + " stop" );
}

void restart_service( const std::string& service )
{
  if( service.empty() )
  {
    throw std::invalid_argument{ "Invalid service name" };
  }

  details::execute_sys_command( "service " + service + " restart" );
}

void reload_service( const std::string& service )
{
  if( service.empty() )
  {
    throw std::invalid_argument{ "Invalid service name" };
  }

  details::execute_sys_command( "service " + service + " reload" );
}

bool service_is_running( const std::string& service )
{
    if( service.empty() )
    {
      throw std::invalid_argument{ "Invalid service name" };
    }

    std::string responce{} details::execute_sys_command( "service " + service + " status" ) };
    return ( responce.find( "is running" ) != std::string::npos );
}

}// service

}// sys

}// utils
