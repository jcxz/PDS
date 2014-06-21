#include "compat.h"
#include "io.h"

//#include <pthread.h>


namespace io {

void __attribute__((optimize("O0"))) touchPages(const unsigned char *file_data, size_t size)
{
  volatile unsigned char byte = 0;
  long pagesize = sysconf(_SC_PAGE_SIZE);

  //struct sched_param param;
  //param.sched_priority = 0;

  //pthread_setschedparam(pthread_self(), SCHED_IDLE, &param);

  for (size_t i = 0; i < size; i += pagesize)
  {
    byte = file_data[i];
  }
}

} // End of namespace io
