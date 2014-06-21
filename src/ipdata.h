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

#ifndef IPDATA_H
#define IPDATA_H

#include "flowdata.h"

#include <ostream>



class IPData
{
  public:
    IPData(void)
      : m_packets(0)
      , m_bytes(0)
    {
    }

    IPData(uint64_t packets, uint64_t bytes)
      : m_packets(packets)
      , m_bytes(bytes)
    {
    }

    uint64_t packets(void) const { return m_packets; }
    uint64_t bytes(void) const { return m_bytes; }

    void setPackets(uint64_t packets) { m_packets = packets; }
    void setBytes(uint64_t bytes) { m_bytes = bytes; }

    IPData & operator+=(const struct flow & f)
    {
      m_packets += __builtin_bswap64(f.packets);
      m_bytes += __builtin_bswap64(f.bytes);
      return *this;
    }

    IPData & operator+=(const IPData & other)
    {
      m_packets += other.m_packets;
      m_bytes += other.m_bytes;
      return *this;
    }

    friend std::ostream & operator<<(std::ostream & os, const IPData & data)
    {
      return os << data.packets() << ' ' << data.bytes();
    }

  private:
    uint64_t m_packets;
    uint64_t m_bytes;
};


class MixedIPData
{
  public:
    MixedIPData(void)
      : m_packets(0)
      , m_bytes(0)
    {
    }

    MixedIPData(uint64_t packets, uint64_t bytes)
      : m_packets(packets)
      , m_bytes(bytes)
    {
    }

    uint64_t packets(void) const { return m_packets & (~(uint64_t(1) << 63)); }
    uint64_t bytes(void) const { return m_bytes; }

    void setPackets(uint64_t packets) { m_packets = packets; }
    void setBytes(uint64_t bytes) { m_bytes = bytes; }

    bool isIPv6(void) const { return !isIPv4(); }
    bool isIPv4(void) const { return (m_packets & (uint64_t(1) << 63)); }

    MixedIPData & operator+=(const struct flow & f)
    {
      m_packets += __builtin_bswap64(f.packets);
      // toto s tym __builtin_bswap32 je celkom hnusny hack, toto by malo byt osetrene nejako krajsie
      m_packets |= (uint64_t(f.sa_family == __builtin_bswap32(AF_INET)) << 63);
      m_bytes += __builtin_bswap64(f.bytes);
      return *this;
    }

    MixedIPData & operator+=(const MixedIPData & other)
    {
      uint64_t addr_type = (m_packets & (uint64_t(1) << 63)) | (other.m_packets & (uint64_t(1) << 63));
      m_packets += other.m_packets;
      m_packets |= addr_type;
      m_bytes += other.m_bytes;
      return *this;
    }

    //friend inline void prefetchIPData(void *addr)
    //{
    //  __builtin_prefetch(addr + offsetof(IPData, m_packets), 0, 3);
    //  __builtin_prefetch(addr + offsetof(IPData, m_bytes), 0, 3);
    //}

    friend std::ostream & operator<<(std::ostream & os, const MixedIPData & data)
    {
      return os << data.packets() << ' ' << data.bytes();
    }

  private:
    uint64_t m_packets;
    uint64_t m_bytes;
};

#endif
