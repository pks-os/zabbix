/*
** Copyright (C) 2001-2025 Zabbix SIA
**
** This program is free software: you can redistribute it and/or modify it under the terms of
** the GNU Affero General Public License as published by the Free Software Foundation, version 3.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
** without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU Affero General Public License for more details.
**
** You should have received a copy of the GNU Affero General Public License along with this program.
** If not, see <https://www.gnu.org/licenses/>.
**/

#include "dbupgrade.h"

/*
 * 7.4 development database patches
 */

#ifndef HAVE_SQLITE3

/*static int	DBpatch_7030000(void)
{
	*** first upgrade patch ***
}*/

#endif

DBPATCH_START(7030)

/* version, duplicates flag, mandatory flag */

/*DBPATCH_ADD(7030000, 0, 1)*/

DBPATCH_END()
