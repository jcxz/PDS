#include "compat.h"
#include "params.h"
#include "netflow.h"

#define NO_DEBUG


int main(int argc, char *argv[])
{
  Params params;

  if (!params.parse(argc, argv))
  {
    usage(argv[0]);
    return 1;
  }

#ifndef NO_DEBUG
  std::cerr << params << std::endl;
#endif

  if ((params.aggregation_key == AGGREGATE_SRC_PORT) ||
      (params.aggregation_key == AGGREGATE_DST_PORT))
  {
    netflow::processPorts(params.dir, params.aggregation_key, params.sort_key);
  }
  else
  {
    netflow::processIPs(params);
  }

  return 0;
}
