#include "compat.h"
#include "params.h"

#include <cstring>
#include <cerrno>
//#include <bitset>
#include <getopt.h>



#define CASE(e) case e: return #e;

const char *sortKeyToString(SortKey key)
{
  switch (key)
  {
    CASE(SORT_PACKETS);
    CASE(SORT_BYTES);
    default: return "Unknown sort key";
  }
}


const char *aggregationKeyToString(AggregationKey key)
{
  switch (key)
  {
    CASE(AGGREGATE_SRC_IP);
    CASE(AGGREGATE_DST_IP);
    CASE(AGGREGATE_SRC_IPv4);
    CASE(AGGREGATE_SRC_IPv6);
    CASE(AGGREGATE_DST_IPv4);
    CASE(AGGREGATE_DST_IPv6);
    CASE(AGGREGATE_SRC_PORT);
    CASE(AGGREGATE_DST_PORT);
    default: return "Unknown aggregation key";
  }
}

#undef CASE


namespace {

struct Int32Binary
{
  uint32_t bits;

  explicit Int32Binary(uint32_t bits) : bits(bits) { }

  friend std::ostream & operator<<(std::ostream & os, const Int32Binary & b)
  {
    for (int i = 31; i >= 0; --i)
    {
      os << (((b.bits >> i) & 1) ? '1' : '0');
      if ((i & 7) == 0) os << ',';
    }
    return os;
  }
};

}


static bool parseMaskNum(const char *str, int & res, int min, int max)
{
  if (*str != '/')
  {
    std::cerr << "ERROR: Missing mask number" << std::endl;
    return false;
  }

  str++;

  errno = 0;
  char *endptr = nullptr;
  long int num_ones = ::strtol(str, &endptr, 10);
  if ((errno != 0) || (endptr == str) || (*endptr != '\0'))
  {
    std::cerr << "ERROR: Mask is not a number" << std::endl;
    return false;
  }

  if ((num_ones < min) || (num_ones > max))
  {
    std::cerr << "ERROR: Mask number is not in range <" << min << ", " << max << '>' << std::endl;
    return false;
  }

  res = num_ones;

  return true;
}


bool Params::parseIPv4Mask(const char *str)
{
  ip4_mask = ~((uint32_t) 0U);

  int num_ones = 0;
  if (!parseMaskNum(str, num_ones, 0, 32)) return false;

  //ip_mask = (num_ones >= 32) ? ~((uint32_t) 0U) : (1U << num_ones) - 1U;
  ip4_mask = (num_ones <= 0) ? ((uint32_t) 0U) : ((~((uint32_t) 0U)) << (32 - num_ones));

  return true;
}


bool Params::parseIPv6Mask(const char *str)
{
  ip6_mask[0] = ~((uint32_t) 0U);
  ip6_mask[1] = ~((uint32_t) 0U);
  ip6_mask[2] = ~((uint32_t) 0U);
  ip6_mask[3] = ~((uint32_t) 0U);

  int num_ones = 0;
  if (!parseMaskNum(str, num_ones, 0, 128)) return false;

  if (num_ones > 96)
  {
    ip6_mask[3] = ((~((uint32_t) 0U)) << (32 - num_ones));
  }
  else if (num_ones > 64)
  {
    ip6_mask[2] = ((~((uint32_t) 0U)) << (64 - num_ones));
    ip6_mask[3] = ((uint32_t) 0U);
  }
  else if (num_ones > 32)
  {
    ip6_mask[1] = ((~((uint32_t) 0U)) << (96 - num_ones));
    ip6_mask[2] = ((uint32_t) 0U);
    ip6_mask[3] = ((uint32_t) 0U);
  }
  else if (num_ones > 0)
  {
    ip6_mask[0] = ((~((uint32_t) 0U)) << (128 - num_ones));
    ip6_mask[1] = ((uint32_t) 0U);
    ip6_mask[2] = ((uint32_t) 0U);
    ip6_mask[3] = ((uint32_t) 0U);
  }
  else
  {
    ip6_mask[0] = ((uint32_t) 0U);
    ip6_mask[1] = ((uint32_t) 0U);
    ip6_mask[2] = ((uint32_t) 0U);
    ip6_mask[3] = ((uint32_t) 0U);
  }

  return true;
}


bool Params::parse(int argc, char *argv[])
{
  opterr = 0;     // prevent getopt_long() from printing error messages
  optind = 1;     // reset the position of the first argument to be scanned
  optarg = NULL; 

  int opt = -1;

  // parse options
  while ((opt = getopt(argc, argv, "f:a:s:")) != -1)
  {
    switch (opt)
    {
      case 'f': dir = optarg; break;

      case 'a':
        if (strcmp(optarg, "srcip") == 0) aggregation_key = AGGREGATE_SRC_IP;
        else if (strcmp(optarg, "dstip") == 0) aggregation_key = AGGREGATE_DST_IP;
        else if (strncmp(optarg, "srcip4", 6) == 0)
        {
          aggregation_key = AGGREGATE_SRC_IPv4;
          if (!parseIPv4Mask(optarg + 6)) return false;
        }
        else if (strncmp(optarg, "dstip4", 6) == 0)
        {
          aggregation_key = AGGREGATE_DST_IPv4;
          if (!parseIPv4Mask(optarg + 6)) return false;
        }
        else if (strncmp(optarg, "srcip6", 6) == 0)
        {
          aggregation_key = AGGREGATE_SRC_IPv6;
          if (!parseIPv6Mask(optarg + 6)) return false;
        }
        else if (strncmp(optarg, "dstip6", 6) == 0)
        {
          aggregation_key = AGGREGATE_DST_IPv6;
          if (!parseIPv6Mask(optarg + 6)) return false;
        }
        else if (strcmp(optarg, "srcport") == 0) aggregation_key = AGGREGATE_SRC_PORT;
        else if (strcmp(optarg, "dstport") == 0) aggregation_key = AGGREGATE_DST_PORT;
        else
        {
          std::cerr << "ERROR: Unrecognized aggregation key" << std::endl;
          return false;
        }
        break;

      case 's':
        if (strcmp(optarg, "packets") == 0) sort_key = SORT_PACKETS;
        else if (strcmp(optarg, "bytes") == 0) sort_key = SORT_BYTES;
        else
        {
          std::cerr << "ERROR: Unrecognized sort key" << std::endl;
          return false;
        }
        break;

      default:
        std::cerr << "ERROR: Unrecognized option" << std::endl;
        return false;
    }
  }

  // check required options
  if (dir == nullptr)
  {
    std::cerr << "ERROR: Input directory missing" << std::endl;
    return false;
  }

  if (aggregation_key == AGGREGATE_NONE)
  {
    std::cerr << "ERROR: Aggregation key missing" << std::endl;
    return false;
  }

  return true;
}


std::ostream & operator<<(std::ostream & os, const Params & params)
{
  os << "directory       : " << (params.dir ? params.dir : "NULL") << std::endl;
  os << "aggregation key : " << aggregationKeyToString(params.aggregation_key) << std::endl;
  os << "IPv4 mask       : " << Int32Binary(params.ip4_mask) << std::endl;
  os << "IPv6 mask       : " << Int32Binary(params.ip6_mask[0]) << ' '
                             << Int32Binary(params.ip6_mask[1]) << ' '
                             << Int32Binary(params.ip6_mask[2]) << ' '
                             << Int32Binary(params.ip6_mask[3]) << std::endl;
  //os << "IPv4 mask       : " << std::bitset<32>(params.ip4_mask) << std::endl;
  //os << "IPv6 mask       : " << std::bitset<32>(params.ip6_mask[0]) << ' '
  //                           << std::bitset<32>(params.ip6_mask[1]) << ' '
  //                           << std::bitset<32>(params.ip6_mask[2]) << ' '
  //                           << std::bitset<32>(params.ip6_mask[3]) << std::endl;
  os << "sort key        : " << sortKeyToString(params.sort_key) << std::endl;
  return os;
}


void usage(const char *prog_name)
{
  std::cout << "USAGE: " << prog_name << " -f directory -a aggregation -s sort" << std::endl;
  std::cout << "\t\tdirectory - directory with binary files to be processed" << std::endl;
  std::cout << "\t\taggregation - the aggregation key (srcip|dstip|srcip4/mask|dstip4/mask|srcip6/mask|dstip6/mask)" << std::endl;
  std::cout << "\t\tsort - the sort key (packets|bytes)" << std::endl;
  return;
}
