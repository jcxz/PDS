/**
 * This file defines interface to port aggregation and sorting functionality
 */

#ifndef NETFLOW_H
#define NETFLOW_H

enum AggregationKey;
enum SortKey;
struct Params;


namespace netflow {

bool processPorts(const char *path, AggregationKey aggregation_key, SortKey sort_key);
bool processIPs(const Params & params);

}

#endif
