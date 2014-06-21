#ifndef PARAMS_H
#define PARAMS_H

#include <cstdint>
#include <iostream>


enum SortKey {
  SORT_PACKETS,
  SORT_BYTES,
  SORT_NONE
};

extern const char *sortKeyToString(SortKey key);

enum AggregationKey {
  AGGREGATE_SRC_IP,
  AGGREGATE_DST_IP,
  AGGREGATE_SRC_IPv4,
  AGGREGATE_SRC_IPv6,
  AGGREGATE_DST_IPv4,
  AGGREGATE_DST_IPv6,
  AGGREGATE_SRC_PORT,
  AGGREGATE_DST_PORT,
  AGGREGATE_NONE
};

extern const char *aggregationKeyToString(AggregationKey key);

struct Params
{
  public:
    const char *dir;
    uint32_t ip4_mask;
    uint32_t ip6_mask[4];
    AggregationKey aggregation_key;
    SortKey sort_key;

    Params(void)
      : dir(nullptr)
      , ip4_mask(~uint32_t(0))
      , aggregation_key(AGGREGATE_NONE)
      , sort_key(SORT_NONE)   
    {
      ip6_mask[0] = (~uint32_t(0));
      ip6_mask[1] = (~uint32_t(0));
      ip6_mask[2] = (~uint32_t(0));
      ip6_mask[3] = (~uint32_t(0));
    }

    bool parse(int argc, char *argv[]);

    friend std::ostream & operator<<(std::ostream & os, const Params & params);

  private:
    bool parseIPv4Mask(const char *str);
    bool parseIPv6Mask(const char *str);
};


extern void usage(const char *prog_name);

#endif
