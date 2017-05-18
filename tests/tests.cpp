#define BIN_NAME "LinuxUtilsUnitTests"
#define BOOST_TEST_MODULE BIN_NAME

#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <sstream>
#include <thread>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/test/included/unit_test.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "../impl/execute_sys_command.h"
#include "sys_app_methods.h"
#include "sys_arch_methods.h"
#include "sys_gpio_methods.h"
#include "sys_network_methods.h"
#include "sys_misc_methods.h"
#include "sys_proc_methods.h"
#include "sys_service_methods.h"
#include "sys_file_methods.h"
#include "sys_time_methods.h"
#include "sys_user_methods.h"

using namespace utils::sys;
using namespace utils::sys::details;

boost::property_tree::ptree CONFIG;

#define EXIT_ON_MISMATCH( VAL, ON_ERROR_MESSAGE )    \
    BOOST_WARN( VAL );                            \
    if( !VAL )                                       \
    {                                                \
        BOOST_TEST_MESSAGE( ON_ERROR_MESSAGE );      \
        exit( 1 );                                   \
    }                                                \


#define EXIT_ON_THROW( S )                                                                                       \
    try                                                                                                          \
    {                                                                                                            \
        S;                                                                                                       \
        BOOST_CHECK_IMPL( true, "no exceptions thrown by " BOOST_STRINGIZE( S ), WARN, CHECK_MSG );              \
    }                                                                                                            \
    catch( ... )                                                                                                 \
    {                                                                                                            \
        BOOST_CHECK_IMPL( false, "exiting: exception thrown by " BOOST_STRINGIZE( S ), WARN, CHECK_MSG );        \
        exit( 1 );                                                                                               \
    }                                                                                                            \

struct create_dir_tree_fixture
{
    create_dir_tree_fixture()
    {
        std::string subfolder{ folder + "/folder" };
        std::string file1{ folder + "/file" };
        std::string file2{ subfolder + "/file" };

        namespace bfs = boost::filesystem;
        BOOST_REQUIRE( bfs::create_directory( folder ) );
        BOOST_REQUIRE( bfs::create_directory( subfolder ) );

        {
            std::ofstream o1{ file1 };
            std::ofstream o2{ file2 };
        }

        BOOST_REQUIRE( bfs::exists( file1 ) && bfs::exists( file2 ) );
    }

    ~create_dir_tree_fixture()
    {
        BOOST_REQUIRE_NO_THROW( boost::filesystem::remove_all( folder ) );
    }

    std::string folder{ "system_test_folder" };
};

// Load and check args
BOOST_AUTO_TEST_CASE( args_check )
{
    BOOST_TEST_MESSAGE( "--------------\nCHECK ARGS" );

    EXIT_ON_MISMATCH( ( boost::framework::master_test_suite().argc >= 2 ), "No config file specified" );

    char** argv{ boost::framework::master_test_suite().argv };
    std::string config_file{ argv[ 1 ] };

    EXIT_ON_THROW( read_json( config_file, CONFIG ) );

    EXIT_ON_THROW( CONFIG.get< std::string >( "user.test_user_name" ) );
    EXIT_ON_THROW( CONFIG.get< std::string >( "misc.binary_partition" ) );
    EXIT_ON_THROW( CONFIG.get< std::string >( "iface.name" ) );
    EXIT_ON_THROW( CONFIG.get< std::string >( "iface.ip" ) );
    EXIT_ON_THROW( CONFIG.get< std::string >( "iface.gw" ) );
    EXIT_ON_THROW( CONFIG.get< std::string >( "iface.mac" ) );
    EXIT_ON_THROW( CONFIG.get< uint >( "iface.mtu" ) );
    EXIT_ON_THROW( CONFIG.get< uint >( "iface.speed" ) );
    EXIT_ON_THROW( CONFIG.get< uint >( "iface.duplex" ) );
    EXIT_ON_THROW( CONFIG.get< uint >( "iface.type" ) );
    EXIT_ON_THROW( CONFIG.get< uint >( "iface.enabled" ) );
    EXIT_ON_THROW( CONFIG.get< double >( "time.offset" ) );
    EXIT_ON_THROW( CONFIG.get< std::string >( "time.offset_str" ) );
    EXIT_ON_THROW( CONFIG.get< std::string >( "app.dir" ) );
}

BOOST_AUTO_TEST_CASE( test_execute_sys_command )
{
  BOOST_TEST_MESSAGE( "--------------\nEXEC_SYS_COMMAND" );

  BOOST_REQUIRE_THROW( execute_sys_command( "" ), std::invalid_argument );

  std::string echo{ "test" };
  std::string out;
  BOOST_REQUIRE_NO_THROW( out = execute_sys_command( std::string{ "echo " } + echo ) );

  BOOST_REQUIRE( out == echo );
}

BOOST_AUTO_TEST_CASE( test_app_path_dir )
{
  BOOST_TEST_MESSAGE( "--------------\nAPP" );

  std::string app_dir{ CONFIG.get< std::string >( "app.dir" ) };
  std::string app_path{ app_dir + "/" + BIN_NAME };

  BOOST_REQUIRE( app::get_application_path() == app_path );
  BOOST_REQUIRE( app::get_application_dir() == app_dir );
}

//// Test SysArchMethods.h

BOOST_FIXTURE_TEST_SUITE( arch_suite, create_dir_tree_fixture )

BOOST_AUTO_TEST_CASE( test_create_extract_arch )
{
  BOOST_TEST_MESSAGE( "--------------\nARCH" );

  std::vector< std::string > paths{ folder };
  std::string arch_name{ "test.tar.gz" };

  BOOST_REQUIRE_THROW( arch::create_archive( paths, "", arch::arch_type::tar_gz ), std::invalid_argument );
  BOOST_REQUIRE_THROW( arch::create_archive( {}, arch_name, arch::arch_type::tar_gz ), std::invalid_argument );
  BOOST_REQUIRE_THROW( arch::create_archive( paths, "", arch::arch_type::_7z ), std::invalid_argument );
  BOOST_REQUIRE_THROW( arch::create_archive( {}, arch_name, arch::arch_type::_7z ), std::invalid_argument );

  BOOST_REQUIRE_THROW( arch::extract_archive( arch_name, "", arch::arch_type::tar_gz ), std::invalid_argument );
  BOOST_REQUIRE_THROW( arch::extract_archive( "", folder, arch::arch_type::tar_gz ), std::invalid_argument );
  BOOST_REQUIRE_THROW( arch::extract_archive( arch_name, "", arch::arch_type::_7z ), std::invalid_argument );
  BOOST_REQUIRE_THROW( arch::extract_archive( "", folder, arch::arch_type::_7z ), std::invalid_argument );


  enum type{ tar_gz, _7z };

  for( int format{ tar_gz }; format <= _7z; ++format )
  {
      std::string arch_name{ format == tar_gz? "arch.tar.gz" : "arch.7z" };
      arch::arch_type arch_type{ format == tar_gz? arch::arch_type::tar_gz : arch::arch_type::_7z };
      uintmax_t arch_size{ 0 };

      BOOST_SCOPE_EXIT( &arch_name ){ std::remove( arch_name.c_str() ); } BOOST_SCOPE_EXIT_END

      // create_archive
      BOOST_REQUIRE_NO_THROW( arch_size = arch::create_archive( paths, arch_name, arch_type ) );
      BOOST_REQUIRE( arch_size != 0 );

      // delete sources
      BOOST_REQUIRE_NO_THROW( boost::filesystem::remove_all( folder ) );

      // extract_archive
      BOOST_REQUIRE_NO_THROW( arch::extract_archive( arch_name, folder, arch_type ) );
      for( const std::string& path : paths )
      {
          BOOST_REQUIRE( boost::filesystem::exists( path ) );
      }
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE( test_gpio )
{
    BOOST_TEST_MESSAGE( "--------------\nGPIO" );

    uint32_t line_num{ 214 };
    int initial_line_status{ -1 };
    gpio::gpio_direction initial_direction{ gpio::gpio_direction::out };
    bool initally_enabled{ false };

    BOOST_SCOPE_EXIT( &initial_line_status )
    {
        if( initial_line_status != -1 )
        {
            std::string command{ std::string{ "echo " } +
                                 std::to_string( initial_line_status ) +
                                         " > /sys/class/gpio/gpio214/value" };

            ::system( command.c_str() );
        }

        ::system( "echo \"out\" > /sys/class/gpio/gpio214/direction" );
    }
    BOOST_SCOPE_EXIT_END

    // disable_gpio_line
    BOOST_REQUIRE_NO_THROW( gpio::disable_gpio_line( line_num ) );
    BOOST_REQUIRE( gpio::gpio_line_enabled( line_num ) == false );

    // enable_gpio_line
    BOOST_REQUIRE_NO_THROW( gpio::enable_gpio_line( line_num, gpio::gpio_direction::in ) );
    BOOST_REQUIRE( gpio::gpio_line_enabled( line_num ) == true );
    BOOST_REQUIRE( gpio::get_gpio_line_direction( line_num ) == gpio::gpio_direction::in );

    // get initial line status to restore it
    BOOST_REQUIRE_NO_THROW( initial_line_status = gpio::get_gpio_line_status( line_num ) );

    // set_gpio_line_status
    int set_line_status{ -1 };
    for( int line_status{ 0 }; line_status <= 1; ++line_status )
    {
        BOOST_REQUIRE_NO_THROW( gpio::set_gpio_line_direction( line_num, gpio::gpio_direction::out ) );
        BOOST_REQUIRE( gpio::get_gpio_line_direction( line_num ) == gpio::gpio_direction::out );
        BOOST_REQUIRE_NO_THROW( gpio::set_gpio_line_status( line_num, line_status ) );

        BOOST_REQUIRE_NO_THROW( gpio::set_gpio_line_direction( line_num, gpio::gpio_direction::in ) );
        BOOST_REQUIRE( gpio::get_gpio_line_direction( line_num ) == gpio::gpio_direction::in );
        BOOST_REQUIRE_NO_THROW( set_line_status = gpio::get_gpio_line_status( line_num ) );
        //std::cout<<set_line_status <<std::endl;
        BOOST_REQUIRE( set_line_status == line_status );
    }
}

BOOST_AUTO_TEST_CASE( test_iface )
{
    BOOST_TEST_MESSAGE( "--------------\nIFACE" );

    std::string ip;

    std::string test_iface_name{ CONFIG.get< std::string >( "iface.name" ) };
    std::string test_iface_ip{ CONFIG.get< std::string >( "iface.ip" ) };
    std::string test_iface_gw{ CONFIG.get< std::string >( "iface.gw" ) };
    std::string test_iface_mac{ CONFIG.get< std::string >( "iface.mac" ) };
    uint test_iface_mtu{ CONFIG.get< uint >( "iface.mtu" ) };
    uint test_iface_speed{ CONFIG.get< uint >( "iface.speed" ) };
    uint test_iface_duplex{ CONFIG.get< uint >( "iface.duplex" ) };
    uint test_iface_type{ CONFIG.get< uint >( "iface.type" ) };
    uint test_iface_enabled{ CONFIG.get< uint >( "iface.enabled" ) };

    // get_iface_gateway
    std::string gateway;
    BOOST_REQUIRE_THROW( gateway = network::get_iface_gateway( "" ), std::invalid_argument );
    BOOST_REQUIRE_NO_THROW( gateway = network::get_iface_gateway( test_iface_name ) );
    BOOST_REQUIRE( gateway == test_iface_gw );

    // get_iface_type
    int iface_type{ 0 };
    BOOST_REQUIRE_THROW( iface_type = network::get_iface_type( "" ), std::invalid_argument );
    BOOST_REQUIRE_NO_THROW( iface_type = network::get_iface_type( test_iface_name ) );
    BOOST_REQUIRE( iface_type == test_iface_type );

    // get_eth_iface_info
    network::netw_iface_info info;
    BOOST_REQUIRE_THROW( info = network::get_eth_iface_info( "" ), std::invalid_argument );
    BOOST_REQUIRE_NO_THROW( info = network::get_eth_iface_info( test_iface_name ) );

    BOOST_REQUIRE( test_iface_name == info.name );
    BOOST_REQUIRE( test_iface_mac == info.mac );
    BOOST_REQUIRE( test_iface_ip == info.ip );
    BOOST_REQUIRE( test_iface_mac == info.mac );
    BOOST_REQUIRE( test_iface_mtu == info.mtu );
    BOOST_REQUIRE( test_iface_speed == info.speed );
    BOOST_REQUIRE( test_iface_duplex == info.duplex );
    BOOST_REQUIRE( test_iface_type == info.type );

    // get_eth_ifaces
    // Assuming we only have eth & lo, lo is omitted
    std::string ifaces;
    BOOST_REQUIRE_NO_THROW( ifaces = execute_sys_command( "ifconfig -a | sed 's/[ \t].*//;/^$/d'" ) );

    std::vector< std::string > ifaces_list;
    boost::split( ifaces_list, ifaces, boost::is_any_of( "\n" ), boost::token_compress_on );

    auto ifaces_list2 = network::get_eth_ifaces();
    BOOST_REQUIRE( ifaces_list.size() - 1 == ifaces_list2.size() ); // -1 because of lo
}

BOOST_AUTO_TEST_CASE( test_misc )
{
    BOOST_TEST_MESSAGE( "--------------\nMISC" );

    // hardware_concurrency
    uint64_t hc{ 0 };
    BOOST_REQUIRE_NO_THROW( hc = hardware_concurrency() );
    BOOST_REQUIRE( hc == std::thread::hardware_concurrency() );

    // get_partition_by_path
    std::string part;
    BOOST_REQUIRE_THROW( file::get_partition_by_path( "" ), std::invalid_argument );
    BOOST_REQUIRE_NO_THROW( part = file::get_partition_by_path( app::get_application_path() ) );
    BOOST_REQUIRE( part == CONFIG.get< std::string >( "misc.binary_partition" ) );
}

BOOST_AUTO_TEST_CASE( test_proc )
{
    BOOST_TEST_MESSAGE( "--------------\nPROC" );

    std::vector< pid_t > pids;

    BOOST_REQUIRE_NO_THROW( pids = proc::get_pids( BIN_NAME ) );
    BOOST_REQUIRE( pids.size() != 0 );

    std::string user_name{ CONFIG.get< std::string >( "user.test_user_name" ) };
    std::string file_name{ "Resources/loop" };
    std::string proc_name{ "loop" };

    BOOST_REQUIRE( boost::filesystem::is_regular_file( file_name ) );

    // kill by pid
    {
        std::thread t{ []{ system( "nohup Resources/loop > /dev/null &" ); } };
        BOOST_SCOPE_EXIT( &t )
        {
            system( "pkill loop" );
            t.join();
        }BOOST_SCOPE_EXIT_END

        std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );

        BOOST_REQUIRE_NO_THROW( pids = proc::get_pids( proc_name ) );
        BOOST_REQUIRE( pids.size() == 1 );
        BOOST_REQUIRE_NO_THROW( proc::kill_by_pid( pids[ 0 ], SIGKILL ) );
        BOOST_REQUIRE_NO_THROW( pids = proc::get_pids( proc_name ) );
        BOOST_REQUIRE( pids.empty() );
    }

    // kill by procname
    {
        std::thread t{ []{ system( "nohup Resources/loop > /dev/null &" ); } };
        BOOST_SCOPE_EXIT( &t ){ t.join(); } BOOST_SCOPE_EXIT_END
        std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );

        BOOST_REQUIRE_NO_THROW( pids = proc::get_pids( proc_name ) );
        BOOST_REQUIRE( pids.size() == 1 );
        BOOST_REQUIRE_NO_THROW( proc::kill_by_procname( proc_name, SIGKILL ) );
        BOOST_REQUIRE_NO_THROW( pids = proc::get_pids( proc_name ) );
        BOOST_REQUIRE( pids.empty() );
    }
}

BOOST_AUTO_TEST_CASE( test_service_start_stop_restart_reload )
{
    BOOST_TEST_MESSAGE( "--------------\nService" );

    // Invalid cases
    BOOST_REQUIRE_THROW( service::stop_service( "" ), std::invalid_argument );
    BOOST_REQUIRE_THROW( service::start_service( "" ), std::invalid_argument );
    BOOST_REQUIRE_THROW( service::restart_service( "" ), std::invalid_argument );
    BOOST_REQUIRE_THROW( service::reload_service( "" ), std::invalid_argument );

    std::string out;
    BOOST_SCOPE_EXIT( &out ){ ::system( "sudo service ntp start" ); }BOOST_SCOPE_EXIT_END

    // stop_service
    BOOST_REQUIRE_NO_THROW( service::stop_service( "ntp" ) );
    BOOST_REQUIRE_NO_THROW( out = execute_sys_command( "sudo service ntp status" ) );
    BOOST_REQUIRE( out.find( "is not running" ) != std::string::npos );

    // start_service
    BOOST_REQUIRE_NO_THROW( service::start_service( "ntp" ) );
    BOOST_REQUIRE_NO_THROW( out = execute_sys_command( "service ntp status" ) );
    BOOST_REQUIRE( out.find( "is running" ) != std::string::npos );

    // restart_service
    BOOST_REQUIRE_NO_THROW( service::restart_service( "ntp" ) );
    BOOST_REQUIRE_NO_THROW( out = execute_sys_command( "service ntp status" ) );
    BOOST_REQUIRE( out.find( "is running" ) != std::string::npos );

    // reload_service
    BOOST_REQUIRE_NO_THROW( service::reload_service( "ntp" ) );
    BOOST_REQUIRE_NO_THROW( out = execute_sys_command( "service ntp status" ) );
    BOOST_REQUIRE( out.find( "is running" ) != std::string::npos );
}

BOOST_AUTO_TEST_CASE( test_set_sys_time )
{
    BOOST_TEST_MESSAGE( "--------------\nTime Set" );

    // Invalid cases
    BOOST_REQUIRE_THROW( time::set_sys_time( 1990, 13, 1, 1, 1, 1 ), std::invalid_argument );
    BOOST_REQUIRE_THROW( time::set_sys_time( 1990, 1, 50, 1, 1, 1 ), std::invalid_argument );
    BOOST_REQUIRE_THROW( time::set_sys_time( 1990, 1, 1, 50, 1, 1 ), std::invalid_argument );
    BOOST_REQUIRE_THROW( time::set_sys_time( 1990, 1, 1, 1, 100, 1 ), std::invalid_argument );
    BOOST_REQUIRE_THROW( time::set_sys_time( 1990, 1, 1, 1, 1, 100 ), std::invalid_argument );

    time_t seconds_past_epoch;
    BOOST_SCOPE_EXIT( &seconds_past_epoch )
    {
        ::system( "sudo ntpdate -s time.nist.gov" );
        service::start_service( "ntp" );
    }
    BOOST_SCOPE_EXIT_END

    BOOST_REQUIRE_NO_THROW( service::stop_service( "ntp" ) );

    BOOST_REQUIRE_NO_THROW( time::set_sys_time( 2000, 10, 10, 10, 10, 10 ) );

    seconds_past_epoch = ::time( 0 );
    tm* now{ localtime( &seconds_past_epoch ) };
    BOOST_REQUIRE( now->tm_year + 1900 == 2000 );
    BOOST_REQUIRE( now->tm_mon == 10 );
    BOOST_REQUIRE( now->tm_mday == 10 );
    BOOST_REQUIRE( now->tm_min == 10 );
    BOOST_REQUIRE( now->tm_sec >= 10 && now->tm_sec <= 40 );
}

BOOST_AUTO_TEST_CASE( test_timezone )
{
    BOOST_TEST_MESSAGE( "--------------\nTime Timezone" );

    std::string time_zone;

    BOOST_SCOPE_EXIT( &time_zone )
    {
        std::string command{ std::string{ "sudo timedatectl set-timezone "} + time_zone  };
        ::system( command.c_str() );
    }
    BOOST_SCOPE_EXIT_END

    // get_timezone()
    std::string test_time_zone{ CONFIG.get< std::string >( "time.timezone" ) };

    BOOST_REQUIRE_NO_THROW( time_zone = time::get_time_zone() );
    BOOST_REQUIRE( test_time_zone == time_zone );

    // get timezone offset
    BOOST_REQUIRE_THROW( time::get_time_zone_offset( "" ), std::invalid_argument );
    std::pair< std::string, double > offset;
    BOOST_REQUIRE_NO_THROW( offset = time::get_time_zone_offset( time_zone ) );

    std::string test_offset_str{ CONFIG.get< std::string >( "time.offset_str" ) };
    double test_offset{ CONFIG.get< double >( "time.offset" ) };
    BOOST_REQUIRE( offset.first == test_offset_str && offset.second == test_offset );

    // get_timezones()
    std::vector< std::string > zones;

    BOOST_REQUIRE_NO_THROW( zones = time::get_time_zones() );
    BOOST_REQUIRE( !zones.empty() && std::find( zones.begin(), zones.end(), time_zone ) != zones.end() );

    // set_timezone()
    BOOST_REQUIRE_THROW( time::set_time_zone( "" ), std::invalid_argument );

    auto it = std::find_if( zones.begin(), zones.end(),
                            [ &time_zone ]( const std::string& curr_zone )->bool
                            { return curr_zone != time_zone; } );

    BOOST_REQUIRE( it != zones.end() );
    std::string other_zone{ *it };

    BOOST_REQUIRE_NO_THROW( time::set_time_zone( other_zone ) );
    BOOST_REQUIRE( time::get_time_zone() == other_zone );
    BOOST_REQUIRE_NO_THROW( time::set_time_zone( time_zone ) );
}

BOOST_AUTO_TEST_CASE( test_user_add_remove )
{
    BOOST_TEST_MESSAGE( "--------------\nUser add remove" );

    std::string user_name{ CONFIG.get< std::string >( "user.test_user_name" ) };
    BOOST_SCOPE_EXIT( &user_name ){
        std::string command{ std::string{ "sudo userdel -f "} + user_name + " && rm -rf /home/plugins/" + user_name  };
        ::system( command.c_str() );
    } BOOST_SCOPE_EXIT_END

    // get_cpu_load_by_user
    BOOST_REQUIRE_THROW( user::get_cpu_load_by_user( "" ), std::invalid_argument );
    BOOST_REQUIRE_THROW( user::get_cpu_load_by_user( user_name ), std::runtime_error );
    BOOST_REQUIRE_NO_THROW( user::get_cpu_load_by_user( "root" ) );
}

BOOST_FIXTURE_TEST_SUITE( chmod_chown_suite, create_dir_tree_fixture )

BOOST_AUTO_TEST_CASE( test_chmod_recurse )
{
    BOOST_TEST_MESSAGE( "--------------\nUser chmod recurse" );

    // Invalid cases
    BOOST_REQUIRE_THROW( user::chmod( "", 0777, user::all, user::recursive::yes ), std::invalid_argument );
    BOOST_REQUIRE_THROW( user::chmod( folder, -01, user::all, user::recursive::yes ), std::invalid_argument );
    BOOST_REQUIRE_THROW( user::chmod( folder, 01000, user::all, user::recursive::yes ), std::invalid_argument );

    // chmod_recurse
    BOOST_REQUIRE_NO_THROW( user::chmod( folder, 0777 , user::all, user::recursive::yes ) );

    namespace bfs = boost::filesystem;
    for( bfs::directory_iterator file{ folder }; file != bfs::directory_iterator{}; ++file )
    {
        bfs::path current{ file->path() };

        bfs::file_status s{ status( current ) };
        BOOST_REQUIRE( s.permissions() == bfs::all_all );
    }
}

BOOST_AUTO_TEST_CASE( test_chown_recurse )
{
    BOOST_TEST_MESSAGE( "--------------\nUser chown recurse" );

    std::pair< std::string, std::string > user_group;

    // Invalid cases
    BOOST_REQUIRE_THROW( user::chown( folder, user_group, user::all, user::recursive::yes ), std::invalid_argument );
    BOOST_REQUIRE_THROW( user::chown( "", user_group , user::all, user::recursive::yes ), std::invalid_argument );

    // chown_recurse
    user_group = { "root", "root" };
    group* g{ getgrnam( user_group.second.c_str() ) };
    BOOST_REQUIRE( g != nullptr );

    BOOST_REQUIRE_NO_THROW( user::chown( folder, user_group , user::all, user::recursive::yes ) );

    namespace bfs = boost::filesystem;
    for( bfs::directory_iterator file{ folder }; file != bfs::directory_iterator{}; ++file )
    {
        struct stat info;
        BOOST_REQUIRE( stat( file->path().string().c_str(), &info ) == 0 );

        passwd* pw{ getpwuid( info.st_uid ) };
        BOOST_REQUIRE( pw != nullptr );
        BOOST_REQUIRE( pw->pw_name == user_group.first );
        BOOST_REQUIRE( pw->pw_gid == g->gr_gid );
    }
}

BOOST_AUTO_TEST_SUITE_END()
