#ifndef __SYS_NETWORK_METHODS_H__
#define __SYS_NETWORK_METHODS_H__

#include <string>
#include <vector>

#include <net/if_arp.h>

namespace utils
{

namespace sys
{

namespace network
{

/// \brief Turn iface up\down
void set_iface_state( const std::string& iface_name, bool on );

/// \brief Returns interface gateway
std::string get_iface_gateway( const std::string& iface_name );

/// \brief Returns interface type
int get_iface_type( const std::string& iface_name );

/// \brief Список режимов настройки (на самом деле еще есть auto и другие, но здесь только те, которые можно выставить из веба)
enum class iface_mode { dynamic_ip, static_ip, unknown };

struct netw_iface_info
{
  std::string name;
  std::string mac;
  std::string ip;
  std::string mask;
  std::string gateway;
  int mtu{ 0 };
  int speed{ 0 };
  int duplex{ 0 };
  int type{ ARPHRD_NONE };
  bool enabled{ false };
  iface_mode mode{ iface_mode::unknown };
};

/// \brief Returns interface infor
netw_iface_info get_eth_iface_info( const std::string& iface_name );

/// \brief Lists netw ifaces
std::vector< netw_iface_info > get_eth_ifaces();

/// \brief Lists dns servers
std::vector< std::string > get_dns_list();

/// \brief Add or delete rule for tcp | dport | INPUT
void set_port_open( const std::string& port, bool is_open );

/// \brief Updates network interfaces data and dns servers for system
void update_network_info( const std::vector< netw_iface_info >& ifaces , const std::vector< std::string >& dns_servers );

/// \brief Apply current iptables
void apply_iptables_settings();

}

}

}


#endif
