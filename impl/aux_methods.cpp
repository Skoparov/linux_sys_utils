#include "../aux_methods.h"

#include <fstream>

namespace utils
{

namespace aux
{

void copy_folder( const std::string& src, const std::string& dst )
{
  namespace fs = boost::filesystem;
  fs::path source{ src };
  fs::path destination{ dst };

  if( !fs::exists( source ) || !fs::is_directory( source ) )
  {
    throw std::invalid_argument{ "Soure does not exists or is not a dir" };
  }

  if( !fs::exists( destination ) && !fs::create_directory( destination ) )
  {
    throw std::invalid_argument{ "Could not create dest dir" };
  }

  for( fs::directory_iterator file{ source }; file != fs::directory_iterator{}; ++file )
  {
    fs::path current{ file->path() };
    if( fs::is_directory( current ) )
    {
        copy_folder( current.string(), ( destination / current.filename() ).string() );
    }
    else
    {
      fs::copy_file( current, destination / current.filename(), fs::copy_option::overwrite_if_exists );
    }
  }
}

std::string read_file( const std::string& path, bool binary, const std::pair< bool, size_t > max_size_limit )
{
  std::ifstream file{ path, binary? std::ios_base::binary : std::ios_base::in };
  if( !file.is_open() )
  {
    throw std::ios_base::failure{ "Failed to open file" + path };
  }

  file.unsetf( std::ios::skipws );

  file.seekg( 0, std::ios::end );
  std::streampos file_size{ file.tellg() };
  file.seekg( 0, std::ios::beg );

  if( max_size_limit.first &&
      file_size > static_cast< std::streampos >( max_size_limit.second ) )
  {
    throw std::invalid_argument{ "File is too large" };
  }

  std::string result;
  result.reserve( file_size );

  result.insert( result.begin(),
                 std::istream_iterator< char >( file ),
                 std::istream_iterator< char >{});

  return result;
}


}// aux

}// utils
