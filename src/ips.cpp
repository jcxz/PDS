#include "compat.h"
#include "ipdata.h"
#include "params.h"
#include "utils.h"
#include "io.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <climits>

#include <algorithm>
#include <unordered_map>
#include <thread>
#include <atomic>

#include <boost/filesystem.hpp>



///////////////////////////////////////////////////////////////////////////////
// Helper functions to be able to hash and print IPv6 addresses

static inline bool operator==(const IPv6Address & lhs, const IPv6Address & rhs)
{
  return (::memcmp(&lhs, &rhs, sizeof(IPv6Address)) == 0);
}


struct IPv6Hash
{
  inline size_t operator()(const IPv6Address & addr) const
  {
    return (addr.data[0] << 3) ^
           (addr.data[1] << 2) ^
           (addr.data[2] << 1) ^
           (addr.data[3]);        // pri IPv4 su uzitocne informacie v tomto trecom policku
                                  // byte order vnutri tychto 32 bitovych integerov je BigEndian,
                                  // cize je to prehodene
  }
};


struct IPv4Hash
{
  inline size_t operator() (const uint32_t ip) const
  {
    //return ip << 1;
    return ip;
  }
};


// IPv6 data structures
typedef std::unordered_map<IPv6Address, IPData, IPv6Hash> tIPv6Hash;
typedef std::pair<typename tIPv6Hash::key_type,
                  typename tIPv6Hash::mapped_type> tIPv6Record;
typedef std::vector<tIPv6Record> tIPv6Vec;

// IPv4 data structures
typedef std::unordered_map<uint32_t, IPData, IPv4Hash> tIPv4Hash;
typedef std::pair<typename tIPv4Hash::key_type,
                  typename tIPv4Hash::mapped_type> tIPv4Record;
typedef std::vector<tIPv4Record> tIPv4Vec;

// Mixed IP data structures
typedef std::unordered_map<IPv6Address, MixedIPData, IPv6Hash> tIPHash;
typedef std::pair<typename tIPHash::key_type,
                  typename tIPHash::mapped_type> tIPRecord;
typedef std::vector<tIPRecord> tIPVec;



///////////////////////////////////////////////////////////////////////////////
// Traits classes for the supported IP address types


struct IPv4CommonTraits
{
  static constexpr uint32_t tag = AF_INET;
  static constexpr uint32_t tag_swapped = __builtin_bswap32(AF_INET);
  static constexpr size_t iplen = INET_ADDRSTRLEN;
  static constexpr size_t overflow = iplen + 20 + 20 + 3 - 1;

  static inline const char *toStr(const tIPv4Record & rec, char *buf)
  {
    return inet_ntop(AF_INET, &rec.first, buf, INET_ADDRSTRLEN);
  }

  typedef tIPv4Hash HashType;
  typedef tIPv4Vec VecType;
  typedef tIPv4Record RecordType;
  typedef uint32_t MaskType;
};

struct IPv6CommonTraits
{
  static constexpr uint32_t tag = AF_INET6;
  static constexpr uint32_t tag_swapped = __builtin_bswap32(AF_INET6);
  static constexpr size_t iplen = INET6_ADDRSTRLEN;
  static constexpr size_t overflow = iplen + 20 + 20 + 3 - 1;

  static inline const char *toStr(const tIPv6Record & rec, char *buf)
  {
    return inet_ntop(AF_INET6, rec.first.data, buf, INET6_ADDRSTRLEN);
  }

  typedef tIPv6Hash HashType;
  typedef tIPv6Vec VecType;
  typedef tIPv6Record RecordType;
  typedef IPv6Mask MaskType;
};


struct IPCommonTraits
{
  static constexpr uint32_t tag = 0;
  static constexpr uint32_t tag_swapped = 0;
  static constexpr size_t iplen = INET6_ADDRSTRLEN;
  static constexpr size_t overflow = iplen + 20 + 20 + 3 - 1;

  static inline const char *toStr(const tIPRecord & rec, char *buf)
  {
    if (rec.second.isIPv4())
    {
      return inet_ntop(AF_INET, &rec.first.data[3], buf, INET_ADDRSTRLEN);
    }
    else
    {
      return inet_ntop(AF_INET6, rec.first.data, buf, INET6_ADDRSTRLEN);
    }
  }

  typedef tIPHash HashType;
  typedef tIPVec VecType;
  typedef tIPRecord RecordType;
  typedef uint32_t MaskType;
};



template <enum AggregationKey>
struct IPTraits;

template <>
struct IPTraits<AGGREGATE_SRC_IPv4> : public IPv4CommonTraits
{
  static constexpr const char *header = "#srcip,packets,bytes\n";
  static constexpr size_t header_len = 21;
};

template <>
struct IPTraits<AGGREGATE_SRC_IPv6> : public IPv6CommonTraits
{
  static constexpr const char *header = "#srcip,packets,bytes\n";
  static constexpr size_t header_len = 21;
};

template <>
struct IPTraits<AGGREGATE_SRC_IP> : public IPCommonTraits
{
  static constexpr const char *header = "#srcip,packets,bytes\n";
  static constexpr size_t header_len = 21;
};

template<>
struct IPTraits<AGGREGATE_DST_IPv4> : public IPv4CommonTraits
{
  static constexpr const char *header = "#dstip,packets,bytes\n";
  static constexpr size_t header_len = 21;
};

template <>
struct IPTraits<AGGREGATE_DST_IPv6> : public IPv6CommonTraits
{
  static constexpr const char *header = "#dstip,packets,bytes\n";
  static constexpr size_t header_len = 21;
};

template <>
struct IPTraits<AGGREGATE_DST_IP> : public IPCommonTraits
{
  static constexpr const char *header = "#dstip,packets,bytes\n";
  static constexpr size_t header_len = 21;
};


///////////////////////////////////////////////////////////////////////////////

namespace {

template <enum AggregationKey Key>
struct ThreadData
{
  std::atomic<bool> run;
  std::atomic<bool> has_work;
  boost::filesystem::path filename;
  //tIPv4Hash hash;
  //uint32_t mask;
  typename IPTraits<Key>::HashType hash;
  typename IPTraits<Key>::MaskType mask;
};

}


#if 0
static inline void printIPv4Addrs(const tIPv4Vec & data)
{
  constexpr size_t overflow = INET_ADDRSTRLEN + 20 + 20 + 3 - 1; // place for the overflow area
  constexpr size_t buf_max = 256 * 1024;  // usable size of the buffer
  char *out_buf = new char[buf_max + overflow];
  int n = data.size();
  int i = 0;
  size_t cnt = 0;

  char ip_str[INET_ADDRSTRLEN];


  while (i < n)
  {
    size_t rem = 0;

    while (i < n)
    {
      if (cnt >= buf_max)
      {
        rem = cnt - buf_max;
        cnt = buf_max;
        break;
      }

      const tIPv4Record & rec = data[i];

      const char *str = inet_ntop(AF_INET, &rec.first, ip_str, INET_ADDRSTRLEN);
      while (*str != 0)
      {
        out_buf[cnt++] = *str++;
      }

      out_buf[cnt++] = ',';

      str = utils::toString<uint64_t, 20>(rec.second.packets());
      while (*str != 0)
      {
        out_buf[cnt++] = *str++;
      }

      out_buf[cnt++] = ',';

      str = utils::toString<uint64_t, 20>(rec.second.bytes());
      while (*str != 0)
      {
        out_buf[cnt++] = *str++;
      }

      out_buf[cnt++] = '\n';

      ++i;
    }

    (void) write(STDOUT_FILENO, out_buf, cnt);

    // copy the left over back to front
    for (size_t j = 0; j < rem; ++j)
    {
      out_buf[j] = out_buf[buf_max + j];
    }

    cnt = rem;
  }

  delete [] out_buf;
}
#else
template <enum AggregationKey Key>
static inline void printIPAddrs(const typename IPTraits<Key>::VecType & data)
{
  //constexpr size_t overflow = INET_ADDRSTRLEN + 20 + 20 + 3 - 1; // place for the overflow area
  constexpr size_t buf_max = 256 * 1024;  // usable size of the buffer
  char *out_buf = new char[buf_max + IPTraits<Key>::overflow];
  int n = data.size();
  int i = 0;
  size_t cnt = 0;

  //char ip_str[INET_ADDRSTRLEN];
  char ip_str[IPTraits<Key>::iplen];


  while (i < n)
  {
    size_t rem = 0;

    while (i < n)
    {
      if (cnt >= buf_max)
      {
        rem = cnt - buf_max;
        cnt = buf_max;
        break;
      }

      //const tIPv4Record & rec = data[i];
      //const char *str = inet_ntop(AF_INET, &rec.first, ip_str, INET_ADDRSTRLEN);

      const typename IPTraits<Key>::RecordType & rec = data[i];
      //const char *str = IPTraits<Key>::toStr(rec.first, ip_str);
      const char *str = IPTraits<Key>::toStr(rec, ip_str);
      while (*str != 0)
      {
        out_buf[cnt++] = *str++;
      }

      out_buf[cnt++] = ',';

      str = utils::toString<uint64_t, 20>(rec.second.packets());
      while (*str != 0)
      {
        out_buf[cnt++] = *str++;
      }

      out_buf[cnt++] = ',';

      str = utils::toString<uint64_t, 20>(rec.second.bytes());
      while (*str != 0)
      {
        out_buf[cnt++] = *str++;
      }

      out_buf[cnt++] = '\n';

      ++i;
    }

    (void) write(STDOUT_FILENO, out_buf, cnt);

    // copy the left over back to front
    for (size_t j = 0; j < rem; ++j)
    {
      out_buf[j] = out_buf[buf_max + j];
    }

    cnt = rem;
  }

  delete [] out_buf;
}
#endif


template <enum AggregationKey Key>
inline void aggregateIPsHelper(const struct flow * __restrict__ file_data,
                               const int32_t num_records,
                               typename IPTraits<Key>::HashType & hash,
                               const typename IPTraits<Key>::MaskType mask);


template <>
inline void aggregateIPsHelper<AGGREGATE_SRC_IPv4>(const struct flow * __restrict__ file_data,
                                                   const int32_t num_records,
                                                   typename IPTraits<AGGREGATE_SRC_IPv4>::HashType & hash,
                                                   const typename IPTraits<AGGREGATE_SRC_IPv4>::MaskType mask)
{
  //constexpr uint32_t ip4_tag = __builtin_bswap32(AF_INET);

  for (int32_t i = 0; i < num_records; ++i)
  {
    //if (file_data[i].sa_family == ip4_tag)  // AF_INET6 for IPv6
    if (file_data[i].sa_family == IPTraits<AGGREGATE_SRC_IPv4>::tag_swapped)  // AF_INET6 for IPv6
    {

      //uint32_t addr = file_data[i].src_addr.__in6_u.__u6_addr32[3] & mask;   // toto bude treba vyriesit v akom byte order je ta IP adresa 
      uint32_t addr = file_data[i].src_addr.data[3] & mask;   // toto bude treba vyriesit v akom byte order je ta IP adresa 
      hash[addr] += file_data[i];
    }
  }
}


template <>
inline void aggregateIPsHelper<AGGREGATE_SRC_IPv6>(const struct flow * __restrict__ file_data,
                                                   const int32_t num_records,
                                                   typename IPTraits<AGGREGATE_SRC_IPv6>::HashType & hash,
                                                   const typename IPTraits<AGGREGATE_SRC_IPv6>::MaskType mask)
{
  IPv6Address addr;

  for (int32_t i = 0; i < num_records; ++i)
  {
    if (file_data[i].sa_family == IPTraits<AGGREGATE_SRC_IPv6>::tag_swapped)
    {
      addr.data[0] = file_data[i].src_addr.data[0] & mask.data[0];
      addr.data[1] = file_data[i].src_addr.data[1] & mask.data[1];
      addr.data[2] = file_data[i].src_addr.data[2] & mask.data[2];
      addr.data[3] = file_data[i].src_addr.data[3] & mask.data[3];
      hash[addr] += file_data[i];
    }
  }
}


template <>
inline void aggregateIPsHelper<AGGREGATE_SRC_IP>(const struct flow * __restrict__ file_data,
                                                 const int32_t num_records,
                                                 typename IPTraits<AGGREGATE_SRC_IP>::HashType & hash,
                                                 const typename IPTraits<AGGREGATE_SRC_IP>::MaskType mask)
{
  (void) mask;

  for (int32_t i = 0; i < num_records; ++i)
  {
    hash[file_data[i].src_addr] += file_data[i];
  }
}


template <>
inline void aggregateIPsHelper<AGGREGATE_DST_IPv4>(const struct flow * __restrict__ file_data,
                                                   const int32_t num_records,
                                                   typename IPTraits<AGGREGATE_DST_IPv4>::HashType & hash,
                                                   const typename IPTraits<AGGREGATE_DST_IPv4>::MaskType mask)
{
  for (int32_t i = 0; i < num_records; ++i)
  {
    if (file_data[i].sa_family == IPTraits<AGGREGATE_DST_IPv4>::tag_swapped)
    {
      uint32_t addr = file_data[i].dst_addr.data[3] & mask;
      hash[addr] += file_data[i];
    }
  }
}


template <>
inline void aggregateIPsHelper<AGGREGATE_DST_IPv6>(const struct flow * __restrict__ file_data,
                                                   const int32_t num_records,
                                                   typename IPTraits<AGGREGATE_DST_IPv6>::HashType & hash,
                                                   const typename IPTraits<AGGREGATE_DST_IPv6>::MaskType mask)
{
  IPv6Address addr;

  for (int32_t i = 0; i < num_records; ++i)
  {
    if (file_data[i].sa_family == IPTraits<AGGREGATE_DST_IPv6>::tag_swapped)
    {
      addr.data[0] = file_data[i].dst_addr.data[0] & mask.data[0];
      addr.data[1] = file_data[i].dst_addr.data[1] & mask.data[1];
      addr.data[2] = file_data[i].dst_addr.data[2] & mask.data[2];
      addr.data[3] = file_data[i].dst_addr.data[3] & mask.data[3];
      hash[addr] += file_data[i];
    }
  }
}


template <>
inline void aggregateIPsHelper<AGGREGATE_DST_IP>(const struct flow * __restrict__ file_data,
                                                 const int32_t num_records,
                                                 typename IPTraits<AGGREGATE_DST_IP>::HashType & hash,
                                                 const typename IPTraits<AGGREGATE_DST_IP>::MaskType mask)
{
  (void) mask;

  for (int32_t i = 0; i < num_records; ++i)
  {
    hash[file_data[i].dst_addr] += file_data[i];
  }
}


template <enum AggregationKey Key>
static void threadAggregation(ThreadData<Key> *data)
{
  //std::thread worker;
  io::FileMapper mapper;

  while (data->run)
  {
    if (data->has_work)
    {
      if (!mapper.remap(data->filename.c_str()))
      {
        FLOW_WARN("Failed to open file %s\n", data->filename.c_str());
        data->has_work = false;
        continue;
      }
      
      struct flow *file_data = mapper.data<struct flow>();
      size_t file_size = mapper.size();
      size_t num_records = file_size / sizeof(struct flow);
      
      //worker = std::thread(&io::touchPages, (unsigned char *) file_data, file_size);
      
      //hash.reserve(num_records >> 3);
      data->hash.reserve(num_records);

      //aggregateIPsHelper<AGGREGATE_SRC_IPv4>(file_data, num_records, data->hash, data->mask);
      aggregateIPsHelper<Key>(file_data, num_records, data->hash, data->mask);

      //worker.join();
      
      data->has_work = false;
    }
  }
}


#if 0
static inline bool aggregateSrcIPv4(const char *path, uint32_t mask)
{
  // TODO: masku dat do spravneho byte order

  using namespace boost::filesystem;

  tIPv4Hash hash;
  std::thread worker;
  io::FileMapper mapper;

  // aggregate the data
  if (is_regular_file(path))
  {
    if (!mapper.remap(path))
    {
      FLOW_ERROR("Failed to open file %s\n", path);
      return false;
    }

    struct flow *file_data = mapper.data<struct flow>();
    size_t file_size = mapper.size();
    size_t num_records = file_size / sizeof(struct flow);

    worker = std::thread(&io::touchPages, (unsigned char *) file_data, file_size);

    //aggregateSrcIPv4Helper(file_data, num_records, hash, mask);
    aggregateIPsHelper<AGGREGATE_SRC_IPv4>(file_data, num_records, hash, mask);

    worker.join();
  }
  else
  {
    int nthreads = std::thread::hardware_concurrency();
    std::thread *worker = new std::thread[nthreads];
    ThreadData<AGGREGATE_SRC_IPv4> *td = new ThreadData<AGGREGATE_SRC_IPv4>[nthreads];

    for (int i = 0; i < nthreads; ++i)
    {
      td[i].run = true;
      td[i].has_work = false;
      td[i].mask = mask;
    }

    for (int i = 0; i < nthreads; ++i)
    {
      worker[i] = std::thread(&threadAggregation<AGGREGATE_SRC_IPv4>, (ThreadData<AGGREGATE_SRC_IPv4> *) (td + i));
    }

    recursive_directory_iterator dir(path);
    recursive_directory_iterator end;

    while (dir != end)
    {
      if (is_regular_file(*dir))
      {
        int i = 0;
        while (true)
        {
          if (!td[i].has_work)
          {
            td[i].filename = dir->path();
            td[i].has_work = true;
            break;
          }
          ++i;
          if (i >= nthreads)
          {
            i = 0;
          }
        }
      }
      ++dir;
    }

    for (int i = 0; i < nthreads; ++i)
    {
      td[i].run = false;
      worker[i].join();
    }

    for (int i = 0; i < nthreads; ++i)
    {
      for (const auto & it : td[i].hash)
      {
        hash[it.first] += it.second;
      }
    }

    delete [] td;
    delete [] worker;
  }

  // sort and print the data
  tIPv4Vec v(hash.begin(), hash.end());

  //typedef std::pair<typename tIPv4Hash::key_type,
  //                  typename tIPv4Hash::mapped_type> tData;

  //std::vector<tData> v(hash.begin(), hash.end());
  //std::vector<tData> v;
  //v.reserve(hash.size());
  //v.assign(hash.begin(), hash.end());

  std::sort(v.begin(), v.end(), [](const tIPv4Record & lhs, const tIPv4Record & rhs) {
    return lhs.second.bytes() > rhs.second.bytes();
  });

  //printIPv4Addrs(v);
  printIPAddrs<AGGREGATE_SRC_IPv4>(v);

  return true;
}
#else
template <enum AggregationKey Key>
static inline bool aggregateIPs(const char *path, typename IPTraits<Key>::MaskType mask, enum SortKey sort_key)
{
  // TODO: masku dat do spravneho byte order

  using namespace boost::filesystem;

  typename IPTraits<Key>::HashType hash;
  std::thread worker;
  io::FileMapper mapper;

  // aggregate the data
  if (is_regular_file(path))
  {
    if (!mapper.remap(path))
    {
      FLOW_ERROR("Failed to open file %s\n", path);
      return false;
    }

    struct flow *file_data = mapper.data<struct flow>();
    size_t file_size = mapper.size();
    size_t num_records = file_size / sizeof(struct flow);

    worker = std::thread(&io::touchPages, (unsigned char *) file_data, file_size);

    aggregateIPsHelper<Key>(file_data, num_records, hash, mask);

    worker.join();
  }
  else
  {
    recursive_directory_iterator dir(path);    // create iterator here in case an exception is thrown
    recursive_directory_iterator end;

    int nthreads = std::thread::hardware_concurrency();
    std::thread *worker = new std::thread[nthreads];
    ThreadData<Key> *td = new ThreadData<Key>[nthreads];

    for (int i = 0; i < nthreads; ++i)
    {
      td[i].run = true;
      td[i].has_work = false;
      td[i].mask = mask;
    }

    for (int i = 0; i < nthreads; ++i)
    {
      worker[i] = std::thread(&threadAggregation<Key>, (ThreadData<Key> *) (td + i));
    }

    while (dir != end)
    {
      if (is_regular_file(*dir))
      {
        int i = 0;
        while (true)
        {
          if (!td[i].has_work)
          {
            td[i].filename = dir->path();
            td[i].has_work = true;
            break;
          }
          ++i;
          if (i >= nthreads)
          {
            i = 0;
          }
        }
      }
      ++dir;
    }

    for (int i = 0; i < nthreads; ++i)
    {
      td[i].run = false;
      worker[i].join();
    }

    for (int i = 0; i < nthreads; ++i)
    {
      for (const auto & it : td[i].hash)
      {
        hash[it.first] += it.second;
      }
    }

    delete [] td;
    delete [] worker;
  }

  // sort and print the data
  typename IPTraits<Key>::VecType v(hash.begin(), hash.end());

  if (sort_key == SORT_BYTES)
  {
    std::sort(v.begin(), v.end(), [](const typename IPTraits<Key>::RecordType & lhs,
                                     const typename IPTraits<Key>::RecordType & rhs)
    {
      return lhs.second.bytes() > rhs.second.bytes();
    });
  }
  else if (sort_key == SORT_PACKETS)
  {
    std::sort(v.begin(), v.end(), [](const typename IPTraits<Key>::RecordType & lhs,
                                     const typename IPTraits<Key>::RecordType & rhs)
    {
      return lhs.second.packets() > rhs.second.packets();
    });
  }

  // print results
  (void) write(STDOUT_FILENO, IPTraits<Key>::header, IPTraits<Key>::header_len);
  printIPAddrs<Key>(v);

  return true;
}
#endif


static inline IPv6Mask makeIPv6Mask(const uint32_t mask[4])
{
  IPv6Mask m;
  //m.data[0] = __builtin_bswap32(mask[3]);
  //m.data[1] = __builtin_bswap32(mask[2]);
  //m.data[2] = __builtin_bswap32(mask[1]);
  //m.data[3] = __builtin_bswap32(mask[0]);

  m.data[0] = __builtin_bswap32(mask[0]);
  m.data[1] = __builtin_bswap32(mask[1]);
  m.data[2] = __builtin_bswap32(mask[2]);
  m.data[3] = __builtin_bswap32(mask[3]);
 
  return m;
}


namespace netflow {

bool processIPs(const Params & params)
{
  if (params.aggregation_key == AGGREGATE_SRC_IPv4)
  {
    return aggregateIPs<AGGREGATE_SRC_IPv4>(params.dir, __builtin_bswap32(params.ip4_mask), params.sort_key);
  }
  else if (params.aggregation_key == AGGREGATE_SRC_IPv6)
  {
    return aggregateIPs<AGGREGATE_SRC_IPv6>(params.dir, makeIPv6Mask(params.ip6_mask), params.sort_key);
  }
  else if (params.aggregation_key == AGGREGATE_SRC_IP)
  {
    //return aggregateIPs<AGGREGATE_SRC_IP>(params.dir, makeIPv6Mask(params.ip6_mask), params.sort_key);
    // IP mask is unused in this case
    return aggregateIPs<AGGREGATE_SRC_IP>(params.dir, (uint32_t) 0, params.sort_key);
  }
  else if (params.aggregation_key == AGGREGATE_DST_IPv4)
  {
    return aggregateIPs<AGGREGATE_DST_IPv4>(params.dir, __builtin_bswap32(params.ip4_mask), params.sort_key);
  }
  else if (params.aggregation_key == AGGREGATE_DST_IPv6)
  {
    return aggregateIPs<AGGREGATE_DST_IPv6>(params.dir, makeIPv6Mask(params.ip6_mask), params.sort_key);
  }
  else if (params.aggregation_key == AGGREGATE_DST_IP)
  {
    //return aggregateIPs<AGGREGATE_DST_IP>(params.dir, makeIPv6Mask(params.ip6_mask), params.sort_key);
    // IP mask is unused in this case
    return aggregateIPs<AGGREGATE_DST_IP>(params.dir, (uint32_t) 0, params.sort_key);
  }

  return false;
}

}  // End of netflow namespace
