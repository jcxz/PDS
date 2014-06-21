#include "compat.h"
#include "portdata.h"
#include "params.h"
#include "utils.h"
#include "io.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <climits>

#include <algorithm>
#include <thread>

#include <boost/filesystem.hpp>



namespace {  // Private namespace with helper functions

static inline void printPorts(const int32_t *ports, const PortData *ports_data, const int n)
{
  constexpr size_t overflow = 48;         // place for the overflow area
  constexpr size_t buf_max = 256 * 1024;  // usable size of the buffer
  char *out_buf = new char[buf_max + overflow];
  int i = 0;
  size_t cnt = 0;

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

      int32_t port = ports[i];

      if (ports_data[port].isInvalid())
      {
        ++i;
        continue;
      }

      const char *str = utils::toString<uint16_t, 5>(__builtin_bswap16(port));
      while (*str != 0)
      {
        out_buf[cnt++] = *str++;
      }

      out_buf[cnt++] = ',';

      str = utils::toString<uint64_t, 20>(ports_data[port].packets());
      while (*str != 0)
      {
        out_buf[cnt++] = *str++;
      }

      out_buf[cnt++] = ',';

      str = utils::toString<uint64_t, 20>(ports_data[port].bytes());
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


template<enum AggregationKey Key>
inline void aggregatePortsHelper(const struct flow * __restrict__ file_data,
                                 const int32_t num_records,
                                 PortData * __restrict__ ports_data);


template<>
inline void aggregatePortsHelper<AGGREGATE_SRC_PORT>(const struct flow * __restrict__ file_data,
                                                     const int32_t num_records,
                                                     PortData * __restrict__ ports_data)
{
  int32_t max = int32_t(num_records) - 16;  // to avoid segfault when computing the address to prefetch
  int32_t i = 0;

  while (i < max)
  {
    //prefetchAggregateData(ports_data + file_data[i + 24].src_port);
    prefetchPortData(ports_data + file_data[i + 16].src_port);
    //prefetchAggregateData(ports_data + file_data[i + 8].src_port);
    //prefetchAggregateData(ports_data + file_data[i + 4].src_port);

    uint16_t port = file_data[i].src_port;
    ports_data[port] += file_data[i];

    ++i;
  }

  while (i < num_records)
  {
    uint16_t port = file_data[i].src_port;
    ports_data[port] += file_data[i];

    ++i;
  }
}


template<>
inline void aggregatePortsHelper<AGGREGATE_DST_PORT>(const struct flow * __restrict__ file_data,
                                                     const int32_t num_records,
                                                     PortData * __restrict__ ports_data)
{
  int32_t max = int32_t(num_records) - 16;  // to avoid segfault when computing the address to prefetch
  int32_t i = 0;

  while (i < max)
  {
    //prefetchAggregateData(ports_data + file_data[i + 24].src_port);
    prefetchPortData(ports_data + file_data[i + 16].dst_port);
    //prefetchAggregateData(ports_data + file_data[i + 8].src_port);
    //prefetchAggregateData(ports_data + file_data[i + 4].src_port);

    uint16_t port = file_data[i].dst_port;
    ports_data[port] += file_data[i];

    ++i;
  }

  while (i < num_records)
  {
    uint16_t port = file_data[i].dst_port;
    ports_data[port] += file_data[i];

    ++i;
  }
}


template<enum AggregationKey Key>
static inline bool aggregatePortsFile(const char *path,
                                      PortData *ports_data,
                                      std::thread & worker,
                                      io::FileMapper & mapper)
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

  aggregatePortsHelper<Key>(file_data, num_records, ports_data);

  worker.join();

  return true;
}


template<enum AggregationKey Key>
static inline bool aggregatePorts(const char *path, PortData *ports_data)
{
  using namespace boost::filesystem;

  std::thread worker;
  io::FileMapper mapper;

  if (is_regular_file(path))
  {
    return aggregatePortsFile<Key>(path, ports_data, worker, mapper);
  }
  else
  {
    recursive_directory_iterator dir(path);
    recursive_directory_iterator end;

    while (dir != end)
    {
      aggregatePortsFile<Key>(dir->path().c_str(), ports_data, worker, mapper);
      ++dir;
    }
  }

  return true;
}

}  // End of private namespace


namespace netflow {

bool processPorts(const char *path, AggregationKey aggregation_key, SortKey sort_key)
{
  // initialize datastructures
  constexpr int n = USHRT_MAX + 1;
  const char *header;
  PortData ports_data[n];

  // process the files
  if (aggregation_key == AGGREGATE_SRC_PORT)
  {
    if (!aggregatePorts<AGGREGATE_SRC_PORT>(path, ports_data)) return false;
    header = "#srcport,packets,bytes\n";
  }
  else if (aggregation_key == AGGREGATE_DST_PORT)
  {
    if (!aggregatePorts<AGGREGATE_DST_PORT>(path, ports_data)) return false;
    header = "#dstport,packets,bytes\n";
  }
  else
  {
    return false;
  }

  // initialize for sorting
  int32_t ports[n];
  for (int i = 0; i < n; ++i)
  {
    ports[i] = i;
  }

  // sort data according to bytes
  if (sort_key == SORT_BYTES)
  {
    std::sort(ports, ports + n, [&ports_data](const int32_t lhs, const int32_t rhs) {
      return ports_data[lhs].bytes() > ports_data[rhs].bytes();
    });
  }
  else if (sort_key == SORT_PACKETS)
  {
    std::sort(ports, ports + n, [&ports_data](const int32_t lhs, const int32_t rhs) {
      return ports_data[lhs].packets() > ports_data[rhs].packets();
    });
  }

  // print results
  (void) write(STDOUT_FILENO, header, 23);
  printPorts(ports, ports_data, n);

  return true;
}

} // End of namespace flow
