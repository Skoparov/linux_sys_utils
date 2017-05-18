#ifndef __SYS_USER_METHODS_H__
#define __SYS_USER_METHODS_H__

#include <string>
#include <vector>
#include <sys/stat.h>

namespace utils
{

namespace sys
{

namespace user
{

/// \brief Returns cpu usage by user divided by number of cores
double get_cpu_load_by_user( const std::string& username );

enum target_type{ dir = 1, file = 2, symlink = 4, all = 7 };
enum class deref_symlinks{ yes = 1, no = 0 };
enum class recursive{ yes = 1, no = 0 };

/// \brief Chmod
/// Same as ::chmod, except for recursive feature and possibility to set target type
void chmod( const std::string& target,
            int rights, // must be OCTAL
            target_type type = all,
            const recursive& is_recursive = recursive::no );

/// \brief Chown
/// /// Same as ::chown/lchown, except for recursive feature and possibility to set target type
void chown( const std::string& target,
            const std::pair< std::string, std::string >& user_group,
            target_type type = all,
            const recursive& is_recursive = recursive::no,
            const deref_symlinks& deref_sym_links = deref_symlinks::yes );

}

}

}


#endif
