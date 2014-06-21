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
