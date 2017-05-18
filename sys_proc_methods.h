#ifndef __SYS_PROC_METHODS_H__
#define __SYS_PROC_METHODS_H__

#include <map>
#include <vector>
#include <string>
#include <signal.h>

namespace utils
{

namespace sys
{

namespace proc
{

/// \brief Returns list of pids matching the procname regex and username.
/// If username is empty, it's not taken into account, but proc_name_regex should always be valid.
std::vector< pid_t > get_pids( const std::string& proc_name_regex, const std::string& username = std::string{} );

/// \brief Kills process with the specified pid
int kill_by_pid( pid_t pid, int sig = SIGTERM );

/// \brief Kills all the processes with name matching regex
/// Returns map of pids that were not killed and thr result of kill()
std::map< pid_t, int > kill_by_procname( const std::string& procname_regex, int sig = SIGTERM );

/// \brief Kills all the processes for the user
/// /// Returns map of pids that were not killed and thr result of kill()
std::map< pid_t, int > kill_by_user( const std::string& username, int sig = SIGTERM );

}

}

}


#endif
