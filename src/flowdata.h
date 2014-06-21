#ifndef FLOWDATA_H
#define FLOWDATA_H

#define __STDC_FORMAT_MACROS     // this will enable printf format macros for 64-bit integers
#include <cstdint>

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>


struct IPv6Address
{
  uint32_t data[4];
};

struct IPv6Mask
{
  uint32_t data[4];
};

struct flow
{
  uint32_t sa_family;
  IPv6Address src_addr;
  IPv6Address dst_addr;
  uint16_t src_port;
  uint16_t dst_port;
  uint64_t packets;
  uint64_t bytes;
};

#endif
