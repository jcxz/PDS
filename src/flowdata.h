/*
 * Copyright (C) 2014 Matus Fedorko <xfedor01@stud.fit.vutbr.cz>
 *
 * This file is part of PDS.
 *
 * PDS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PDS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PDS. If not, see <http://www.gnu.org/licenses/>.
 */

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
