#ifndef PORTDATA_H
#define PORTDATA_H

#include "flowdata.h"

#include <cstddef>  // offsetof
#include <ostream>



class PortData
{
  public:
    PortData(void)
      : m_packets(0)
      , m_bytes(0)
    {
    }

    PortData(uint64_t packets, uint64_t bytes)
      : m_packets(packets)
      , m_bytes(bytes)
    {
    }

    uint64_t packets(void) const { return m_packets & (~(uint64_t(1) << 63)); }
    uint64_t bytes(void) const { return m_bytes; }

    void setPackets(uint64_t packets) { m_packets = packets; }
    void setBytes(uint64_t bytes) { m_bytes = bytes; }

    bool isInvalid(void) const { return !isValid(); }
    //{
    //  return !(m_packets & (uint64_t(1) << 63));
    //}

    bool isValid(void) const { return (m_packets & (uint64_t(1) << 63)); }

    PortData & operator+=(const struct flow & f)
    {
      m_packets += __builtin_bswap64(f.packets);
      m_packets |= (uint64_t(1) << 63);
      m_bytes += __builtin_bswap64(f.bytes);
      return *this;
    }

    PortData & operator+=(const PortData & other)
    {
      uint64_t validity = (m_packets & (uint64_t(1) << 63)) | (other.m_packets & (uint64_t(1) << 63));
      m_packets += other.m_packets;
      m_packets |= validity;
      m_bytes += other.m_bytes;
      return *this;
    }

    friend inline void prefetchPortData(void *addr)
    {
      __builtin_prefetch(addr + offsetof(PortData, m_packets), 0, 3);
      __builtin_prefetch(addr + offsetof(PortData, m_bytes), 0, 3);
    }

    friend std::ostream & operator<<(std::ostream & os, const PortData & data)
    {
      return os << data.packets() << ' ' << data.bytes();
    }

  private:
    uint64_t m_packets;
    uint64_t m_bytes;
};

#endif
