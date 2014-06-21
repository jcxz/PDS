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
