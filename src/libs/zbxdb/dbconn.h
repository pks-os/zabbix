/*
** Copyright (C) 2001-2024 Zabbix SIA
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

#ifndef ZABBIX_DBCONN_H
#define ZABBIX_DBCONN_H

#include "zbxsysinc.h"
#include "zbxdb.h"
#include "zbxdbschema.h"

#if defined(HAVE_MYSQL)
#	include "mysql.h"
#	include "errmsg.h"
#	include "mysqld_error.h"
#elif defined(HAVE_ORACLE)
#	include "zbxcrypto.h"
#	include "zbxdbschema.h"
#	include "oci.h"
#elif defined(HAVE_POSTGRESQL)
#	include <libpq-fe.h>
#elif defined(HAVE_SQLITE3)
#	include <sqlite3.h>
#	include <zbxmutexs.h>
#endif

struct zbx_dbconn
{
	int			txn_level;	/* transaction level, nested transactions are not supported */
	int			txn_error;	/* failed transaction */
	int			txn_end_error;	/* transaction result */
	int			connection_failure;

	char			*last_db_strerror;	/* last database error message */
	zbx_err_codes_t		last_db_errcode;

	int			autoincrement;	/* TODO: - must be initialized for proxy, store in config ? */

	int			connect_options;

	int			managed;	/* managed by connection pool */

	const zbx_db_config_t	*config;

#if defined(HAVE_MYSQL)
	MYSQL			*conn;
	int			error_count;
	zbx_uint32_t		ZBX_MYSQL_SVERSION;
	int			ZBX_MARIADB_SFORK;
	int			txn_begin;		/* transaction begin statement is executed */
#elif defined(HAVE_POSTGRESQL)
	PGconn			*conn;
	int			ZBX_TSDB_VERSION;
	zbx_uint32_t		ZBX_PG_SVERSION;
	int 			ZBX_TIMESCALE_COMPRESSION_AVAILABLE;
	int			ZBX_PG_READ_ONLY_RECOVERABLE;
#elif defined(HAVE_SQLITE3)
	sqlite3			*conn;
	zbx_mutex_t		*sqlite_access;
#endif
};


#endif

