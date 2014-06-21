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
