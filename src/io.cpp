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
