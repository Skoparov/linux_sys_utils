#include "../sys_network_methods.h"

#include <stdio.h>
#include <cstdio>
#include <unistd.h>
#include <string.h>
#include <stdexcept>
#include <sstream>
#include <fstream>

#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if_arp.h>

#include <boost/scope_exit.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/host_name.hpp>

#include "../sys_user_methods.h"
#include "../aux_methods.h"
#include "execute_sys_command.h"

#define INTERFACES_FILE "/etc/network/interfaces"
#define RESOLV_CONF_FILE "/etc/resolv.conf"
#define RESOLV_CONF_BASE_FILE "/etc/resolvconf/resolv.conf.d/base"
#define RESOLV_CONF_RUN_FILE "/run/resolvconf/resolv.conf"
namespace utils
{

namespace sys
{

namespace network
{

void set_iface_state( const std::string& iface_name, bool on )
{
  std::string state{ on? "up" : "down" };
  std::string command{ "if" + state + " " + iface_name };
  if( ::system( command.c_str() ) != 0 )
  {
    throw std::runtime_error{ "Could not turn iface " + iface_name + " " + state };
  }
}

std::vector< std::string > get_dns_list()
{
  std::ifstream in{ RESOLV_CONF_FILE };
  if( !in.is_open() )
  {
    throw std::ios_base::failure{ std::string{ "Failed to open file: " } + RESOLV_CONF_FILE + " for read" };
  }

  std::vector< std::string > dns_list;
  std::string pattern{ "nameserver " };

  std::string line;
  while( std::getline( in, line ) )
  {
    if( line.find( pattern ) == 0 )
    {
      line.erase( line.begin(), line.begin() + pattern.length() );

      boost::system::error_code ec;
      boost::asio::ip::address::from_string( line, ec );
      if( !ec )
      {
        dns_list.emplace_back( line );
      }
      else
      {
        throw std::runtime_error{ "Resolv.conf file appears to be corrupted" };
      }
    }
  }

  return dns_list;
}

void update_network_info( const std::vector< netw_iface_info >& ifaces, const std::vector< std::string >& dns_servers )
{
  std::string interfaces_text{ "# This file was generated by Kraftway Smart Video Detector 3\n\n"
                               "auto lo\n"
                               "iface lo inet loopback\n\n"};

  for( const netw_iface_info& iface : ifaces )
  {
    // Чтобы DHCP корректно заработал, необходимо что файл не содержал записи по нужным интерфейсам
    if( iface.mode != iface_mode::dynamic_ip )
    {
      interfaces_text.append( "auto " + iface.name + "\n"
                              "iface " + iface.name + " inet static\n" );

      if( !iface.ip.empty() )
      {
        interfaces_text.append( "address " + iface.ip + "\n" );
      }

      if( !iface.mask.empty() )
      {
        interfaces_text.append( "netmask " + iface.mask + "\n" );
      }

      if( !iface.gateway.empty() )
      {
        interfaces_text.append( "gateway " + iface.gateway + "\n\n" );
      }
    }
  }

  std::string dns_text;
  for( const std::string& dns_server : dns_servers )
  {
    if( !dns_server.empty() )
    {
      dns_text.append( "nameserver " + dns_server + "\n" );
    }
  }

  // write ifaces
  if( !interfaces_text.empty() )
  {
    std::ofstream ifaces_file{ INTERFACES_FILE };
    if( !ifaces_file.is_open() )
    {
      throw std::ios_base::failure{ std::string{ "Could not open file: " } + INTERFACES_FILE + " for write" };
    }

    ifaces_file << interfaces_text;
    ifaces_file.close();
  }

  // write dns
  if( !dns_text.empty() )
  {
    utils::sys::user::chmod( RESOLV_CONF_BASE_FILE, 0666 );
    if( !boost::filesystem::exists( RESOLV_CONF_FILE ) &&
        ( symlink( RESOLV_CONF_RUN_FILE, RESOLV_CONF_FILE ) != 0 ) )
    {
      throw std::runtime_error{ std::string{ "Failed to create symlink: " } + std::strerror(errno) };
    }

    std::ofstream dns_file{ RESOLV_CONF_BASE_FILE };
    if( !dns_file.is_open() )
    {
      throw std::ios_base::failure{ std::string{ "Could not open file: " } + RESOLV_CONF_BASE_FILE + " for write" };
    }

    dns_file << dns_text;
    dns_file.close();

    if( ::system( "resolvconf -u" ) != 0 )
    {
      throw std::runtime_error{ "Failed to update resolvconf" };
    }
  }
}

std::string get_iface_ip_or_any( const std::string& iface_name )
{
  std::string interface_ip;

  char buf[ 1024 ];

  ifconf ifc;
  ifc.ifc_len = sizeof( buf );
  ifc.ifc_buf = buf;

  int sock_all_ifaces{ socket( AF_INET, SOCK_DGRAM, IPPROTO_IP ) };

  if( sock_all_ifaces != -1 && ioctl( sock_all_ifaces, SIOCGIFCONF, &ifc ) != -1 )
  {
    std::string curr_name;

    ifreq* it{ ifc.ifc_req };
    const ifreq* const end{ it + ifc.ifc_len / sizeof( ifreq ) };

    for ( ; it != end; ++it )
    {
      curr_name = it->ifr_name;

     // Ищем адрес, или, если eth_name пустой, берем первый попавшийся, только не "lo"
     if( ( iface_name.length() && curr_name == iface_name ) ||
         ( iface_name.empty() && curr_name != "lo" ) )
     {
        ifreq ifr;
        strcpy( ifr.ifr_name, it->ifr_name );

        /* process ip */
        if( ioctl ( sock_all_ifaces, SIOCGIFADDR, &ifr ) == 0 )
        {
          interface_ip.assign(inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr) );
          break;
        }
      }
    }

    close( sock_all_ifaces );
  }

  if( interface_ip.empty() )
  {
    boost::system::error_code ec;
    interface_ip = boost::asio::ip::host_name( ec );

    if( ec )
    {
      interface_ip = "HOST_IP";
    }
  }

  return interface_ip;
}

int get_iface_type( const std::string& iface_name )
{
  if( iface_name.empty() )
  {
    throw std::invalid_argument{ "Invalid interface" };
  }

  std::string str{ "/sys/class/net/" + iface_name + "/type" };
  std::string type_str{ aux::read_file( str ) };

  return std::stoi( type_str );
}

std::string get_iface_gateway( const std::string& iface_name )
{
  if( iface_name.empty() )
  {
    throw std::invalid_argument{ "Invalid interface" };
  }

  std::stringstream ss;
  ss << "route -n | grep " << iface_name << " | grep 'UG[ \t]' | awk '{print $2}'";

  return details::execute_sys_command( ss.str() );
}

netw_iface_info get_eth_iface_info( const std::string& iface_name )
{
  if( iface_name.empty() )
  {
    throw std::invalid_argument{ "Invalid interface" };
  }

  netw_iface_info result;

  result.name = iface_name;
  result.type = get_iface_type( iface_name );
  result.gateway = get_iface_gateway( iface_name );

  int sock{ socket( PF_INET, SOCK_DGRAM, 0 ) };
  if ( sock != -1 )
  {
    BOOST_SCOPE_EXIT( &sock ){ close( sock ); } BOOST_SCOPE_EXIT_END

    ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy( ifr.ifr_name , iface_name.c_str() , IFNAMSIZ-1 );

    if( ioctl( sock, SIOCGIFFLAGS, &ifr ) != -1 )
    {
      result.enabled = ifr.ifr_flags & IFF_UP;
    }
    else
    {
      throw std::runtime_error{ "SIOCGIFFLAGS ioctl failed, errno: " +
                                 std::string{ std::strerror( errno ) } };
    }

    // mac
    if( ioctl( sock, SIOCGIFHWADDR, &ifr ) != -1 )
    {
      std::array< char, 32 > mac;

      for( int octet = 0, indent = 0; octet < 6; ++octet )
      {
        indent += snprintf( mac.data() + indent,
                            mac.size() - indent - 1,
                            octet? ":%02X" : "%02X",
                            ( unsigned char )ifr.ifr_hwaddr.sa_data[ octet ] );
      }

      mac[ mac.size() - 1 ] = '\0';

      result.mac = mac.data();
    }
    else
    {
      throw std::runtime_error{ "SIOCGIFHWADDR ioctl failed, errno: " +
                                std::string{ std::strerror( errno ) } };
    }

    // mtu
    if( ioctl ( sock, SIOCGIFMTU, &ifr ) != -1 )
    {
      result.mtu = ifr.ifr_mtu;
    }
    else
    {
      throw std::runtime_error{ "SIOCGIFMTU ioctl failed, errno: "+
                                std::string{ std::strerror( errno ) } };
    }

    // ip
    if( ioctl ( sock, SIOCGIFADDR, &ifr ) != -1 )
    {
      result.ip = inet_ntoa( ( ( sockaddr_in* )&ifr.ifr_addr )->sin_addr );
    }
    else
    {
      throw std::runtime_error{ "SIOCGIFADDR ioctl failed, errno: " +
                                std::string{ std::strerror( errno ) } };
    }

    // Mask
    if( ioctl(sock, SIOCGIFNETMASK, &ifr ) != -1 )
    {
      result.mask = inet_ntoa( ( ( sockaddr_in* )&ifr.ifr_netmask )->sin_addr );
    }
    else
    {
      throw std::runtime_error{ "SIOCGIFNETMASK ioctl failed, errno: " +
                                std::string{ std::strerror( errno ) } };
    }

    // speed
    ethtool_cmd edata;
    edata.cmd = ETHTOOL_GSET;
    ifr.ifr_data = reinterpret_cast< char* >( &edata );

    if( ioctl( sock, SIOCETHTOOL, &ifr ) != -1 )
    {
      result.speed = ethtool_cmd_speed( &edata );
      result.duplex = edata.duplex;
    }
    else if( iface_name != "lo" )
    {
      throw std::runtime_error{ "SIOCETHTOOL ioctl failed, errrno: " +
                                 std::string{ std::strerror( errno ) } };
    }

    // mode
    std::unique_ptr< FILE, int( * )( FILE* ) > f{ fopen( INTERFACES_FILE, "r" ), fclose };
    if( !f )
    {
      throw std::ios_base::failure{ " Failed opening /etc/network/interfaces" };
    }

    result.mode = iface_mode::dynamic_ip;
    std::array< char, IFNAMSIZ > dname;
    std::array< char, IFNAMSIZ > inet;
    std::array< char, IFNAMSIZ > mode;
    std::array< char, 1024 > buffer;

    while( !feof( f.get() ) )
    {
      if( fgets( buffer.data(), buffer.size(), f.get() ) )
      {
        sscanf( buffer.data(),"iface %s %s %s", dname.data(), inet.data(), mode.data() );

        if( strcmp( iface_name.c_str(), dname.data() ) == 0 &&
            strcmp( inet.data(), "inet" ) == 0 )
        {
          if( strcmp( mode.data(), "static" ) == 0 )
          {
            result.mode = iface_mode::static_ip;
          }

          break;
        }
      }
    }
  }
  else
  {
    throw std::runtime_error{ "Could not open socket" };
  }

  return result;
}

void set_port_open( const std::string& port, bool is_open )
{
  std::string command{ std::string{ "iptables -" } + ( is_open ? "A" : "D" ) + " INPUT -p tcp --dport " + port + " -j ACCEPT" };
  details::execute_sys_command( command );
}

std::vector< netw_iface_info > get_eth_ifaces()
{
  std::vector< netw_iface_info > result;

  ifaddrs* addr_list{ nullptr };

  if( getifaddrs( &addr_list ) != -1 )
  {
    BOOST_SCOPE_EXIT( addr_list ){ freeifaddrs( addr_list ); } BOOST_SCOPE_EXIT_END

    ifaddrs* curr_if{ addr_list };
    while (curr_if)
    {
      if( curr_if->ifa_addr && curr_if->ifa_addr->sa_family == AF_PACKET )
      {
        std::string name{ curr_if->ifa_name };

        try
        {
          netw_iface_info if_info{ get_eth_iface_info( name ) };
          if( if_info.type == ARPHRD_ETHER )
          {
            if_info.name = name;
            result.emplace_back( if_info );
          }
        }
        catch( const std::exception& e )
        {
          std::string error{ name + " : get_eth_iface_info exception: " + e.what() };
          throw std::runtime_error{ error };
        }
      }

      curr_if = curr_if->ifa_next;
    }
  }
  else
  {
    throw std::runtime_error{ "Could not get ifaces list" };
  }

  return result;
}

void apply_iptables_settings()
{
  details::execute_sys_command( "service iptables.rules apply-acl-hosts" );
  details::execute_sys_command( "service iptables.rules apply-acl-ports" );
}

}// iface

}// sys

}// utils