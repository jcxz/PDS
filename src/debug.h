#ifndef DEBUG_H
#define DEBUG_H

#define FLOW_ERROR(...) fprintf(stderr, "ERROR: " __VA_ARGS__)
#define FLOW_WARN(...) fprintf(stderr, "WARNING: " __VA_ARGS__)
#define FLOW_INFO(...) fprintf(stderr, "NOTE: " __VA_ARGS__)

#ifdef DEBUG
#define FLOW_DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define FLOW_DBG(...)
#endif

#endif
