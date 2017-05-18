#ifndef __SYS_ARCH_METHODS_H__
#define __SYS_ARCH_METHODS_H__

#include <string>
#include <vector>

namespace utils
{

namespace sys
{

namespace arch
{

enum class arch_type{ tar_gz, _7z };

/// \brief Creates tar.gz archive, returns archive size
uintmax_t create_archive( const std::vector< std::string >& paths, const std::string& archive_path, const arch_type& type  );

/// \brief Extract .gz or 7z archive to dest folder
void extract_archive( const std::string& archive_path, const std::string& dest, const arch_type& type );

}

}

}


#endif
