#ifndef __EXEC_SYS_COMMAND_H__
#define __EXEC_SYS_COMMAND_H__

#include <string>
#include <stdexcept>

namespace utils
{

namespace sys
{

namespace details
{

std::string execute_sys_command( const std::string& command );

}

}

}

#endif
