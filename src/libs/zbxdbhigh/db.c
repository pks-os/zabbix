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

#include "zbxdbhigh.h"

#include "zbxthreads.h"
#include "zbxcrypto.h"
#include "zbxnum.h"
#include "zbx_host_constants.h"
#include "zbxalgo.h"
#include "zbxdb.h"
#include "zbxdbschema.h"
#include "zbxstr.h"

#if (!(defined(HAVE_MYSQL_TLS) || defined(HAVE_MARIADB_TLS) || defined(HAVE_POSTGRESQL))) || \
	(!(defined(HAVE_MYSQL_TLS) || defined(HAVE_POSTGRESQL))) || \
	(!(defined(HAVE_MYSQL_TLS) || defined(HAVE_MARIADB_TLS)))
#	include "zbxcfg.h"
#endif

#ifdef HAVE_POSTGRESQL
#	include "zbx_dbversion_constants.h"
#endif

#define ZBX_DB_WAIT_DOWN	10

#define ZBX_MAX_SQL_SIZE	262144	/* 256KB */
#ifndef ZBX_MAX_OVERFLOW_SQL_SIZE
#	define ZBX_MAX_OVERFLOW_SQL_SIZE	ZBX_MAX_SQL_SIZE
#elif 0 != ZBX_MAX_OVERFLOW_SQL_SIZE && \
	(1024 > ZBX_MAX_OVERFLOW_SQL_SIZE || ZBX_MAX_OVERFLOW_SQL_SIZE > ZBX_MAX_SQL_SIZE)
#error ZBX_MAX_OVERFLOW_SQL_SIZE is out of range
#endif

#ifdef HAVE_MULTIROW_INSERT
#	define ZBX_ROW_DL	","
#else
#	define ZBX_ROW_DL	";\n"
#endif

ZBX_PTR_VECTOR_IMPL(db_event, zbx_db_event *)
ZBX_PTR_VECTOR_IMPL(events_ptr, zbx_event_t *)
ZBX_PTR_VECTOR_IMPL(escalation_new_ptr, zbx_escalation_new_t *)
ZBX_PTR_VECTOR_IMPL(item_diff_ptr, zbx_item_diff_t *)
ZBX_PTR_VECTOR_IMPL(trigger_diff_ptr, zbx_trigger_diff_t *)
ZBX_PTR_VECTOR_IMPL(db_field_ptr, zbx_db_field_t *)
ZBX_PTR_VECTOR_IMPL(db_value_ptr, zbx_db_value_t *)

void	zbx_item_diff_free(zbx_item_diff_t *item_diff)
{
	zbx_free(item_diff);
}

int	zbx_item_diff_compare_func(const void *d1, const void *d2)
{
	const zbx_item_diff_t    *id_1 = *(const zbx_item_diff_t **)d1;
	const zbx_item_diff_t    *id_2 = *(const zbx_item_diff_t **)d2;

	ZBX_RETURN_IF_NOT_EQUAL(id_1->itemid, id_2->itemid);

	return 0;
}

int	zbx_trigger_diff_compare_func(const void *d1, const void *d2)
{
	const zbx_trigger_diff_t    *id_1 = *(const zbx_trigger_diff_t **)d1;
	const zbx_trigger_diff_t    *id_2 = *(const zbx_trigger_diff_t **)d2;

	ZBX_RETURN_IF_NOT_EQUAL(id_1->triggerid, id_2->triggerid);

	return 0;
}

static int	connection_failure;

static const zbx_config_dbhigh_t	*zbx_cfg_dbhigh = NULL;

static zbx_dc_get_nextid_func_t				zbx_cb_nextid;

void	zbx_db_close(void)
{
	zbx_db_close_basic();
}

int	zbx_db_validate_config_features(unsigned char program_type, const zbx_config_dbhigh_t *config_dbhigh)
{
	int	err = 0;

#if !(defined(HAVE_MYSQL_TLS) || defined(HAVE_MARIADB_TLS) || defined(HAVE_POSTGRESQL))
	err |= (FAIL == zbx_check_cfg_feature_str("DBTLSConnect", config_dbhigh->config_db_tls_connect,
			"PostgreSQL or MySQL library version that support TLS"));
	err |= (FAIL == zbx_check_cfg_feature_str("DBTLSCAFile", config_dbhigh->config_db_tls_ca_file,
			"PostgreSQL or MySQL library version that support TLS"));
	err |= (FAIL == zbx_check_cfg_feature_str("DBTLSCertFile", config_dbhigh->config_db_tls_cert_file,
			"PostgreSQL or MySQL library version that support TLS"));
	err |= (FAIL == zbx_check_cfg_feature_str("DBTLSKeyFile", config_dbhigh->config_db_tls_key_file,
			"PostgreSQL or MySQL library version that support TLS"));
#endif

#if !(defined(HAVE_MYSQL_TLS) || defined(HAVE_POSTGRESQL))
	if (NULL != config_dbhigh->config_db_tls_connect && 0 == strcmp(config_dbhigh->config_db_tls_connect,
			ZBX_DB_TLS_CONNECT_VERIFY_CA_TXT))
	{
		zbx_error("\"DBTLSConnect\" configuration parameter value '%s' cannot be used: Zabbix %s was compiled"
			" without PostgreSQL or MySQL library version that support this value",
			ZBX_DB_TLS_CONNECT_VERIFY_CA_TXT, get_program_type_string(program_type));
		err |= 1;
	}
#else
	ZBX_UNUSED(program_type);
	ZBX_UNUSED(config_dbhigh);
#endif

#if !(defined(HAVE_MYSQL_TLS) || defined(HAVE_MARIADB_TLS))
	err |= (FAIL == zbx_check_cfg_feature_str("DBTLSCipher", config_dbhigh->config_db_tls_cipher,
			"MySQL library version that support configuration of cipher"));
#endif

#if !defined(HAVE_MYSQL_TLS_CIPHERSUITES)
	err |= (FAIL == zbx_check_cfg_feature_str("DBTLSCipher13", config_dbhigh->config_db_tls_cipher_13,
			"MySQL library version that support configuration of TLSv1.3 ciphersuites"));
#endif

	return 0 != err ? FAIL : SUCCEED;
}

zbx_config_dbhigh_t	*zbx_config_dbhigh_new(void)
{
	zbx_config_dbhigh_t	*config_dbhigh;

	config_dbhigh = (zbx_config_dbhigh_t *)zbx_malloc(NULL, sizeof(zbx_config_dbhigh_t));
	memset(config_dbhigh, 0, sizeof(zbx_config_dbhigh_t));

	return config_dbhigh;
}

void	zbx_config_dbhigh_free(zbx_config_dbhigh_t *config_dbhigh)
{
	zbx_free(config_dbhigh->config_dbhost);
	zbx_free(config_dbhigh->config_dbname);
	zbx_free(config_dbhigh->config_dbschema);
	zbx_free(config_dbhigh->config_dbuser);
	zbx_free(config_dbhigh->config_dbpassword);
	zbx_free(config_dbhigh->config_dbsocket);
	zbx_free(config_dbhigh->config_db_tls_connect);
	zbx_free(config_dbhigh->config_db_tls_cert_file);
	zbx_free(config_dbhigh->config_db_tls_key_file);
	zbx_free(config_dbhigh->config_db_tls_ca_file);
	zbx_free(config_dbhigh->config_db_tls_cipher);
	zbx_free(config_dbhigh->config_db_tls_cipher_13);

	zbx_free(config_dbhigh);
}

void	zbx_init_library_dbhigh(const zbx_config_dbhigh_t *config_dbhigh)
{
	zbx_cfg_dbhigh = config_dbhigh;
}

#if defined(HAVE_MYSQL) || defined(HAVE_POSTGRESQL)
static void	check_cfg_empty_str(const char *parameter, const char *value)
{
	if (NULL != value && 0 == strlen(value))
	{
		zabbix_log(LOG_LEVEL_CRIT, "configuration parameter \"%s\" is defined but empty", parameter);
		exit(EXIT_FAILURE);
	}
}

void	zbx_db_validate_config(const zbx_config_dbhigh_t *config_dbhigh)
{
	check_cfg_empty_str("DBTLSConnect", config_dbhigh->config_db_tls_connect);
	check_cfg_empty_str("DBTLSCertFile", config_dbhigh->config_db_tls_cert_file);
	check_cfg_empty_str("DBTLSKeyFile", config_dbhigh->config_db_tls_key_file);
	check_cfg_empty_str("DBTLSCAFile", config_dbhigh->config_db_tls_ca_file);
	check_cfg_empty_str("DBTLSCipher", config_dbhigh->config_db_tls_cipher);
	check_cfg_empty_str("DBTLSCipher13", config_dbhigh->config_db_tls_cipher_13);

	if (NULL != config_dbhigh->config_db_tls_connect &&
			0 != strcmp(config_dbhigh->config_db_tls_connect, ZBX_DB_TLS_CONNECT_REQUIRED_TXT) &&
			0 != strcmp(config_dbhigh->config_db_tls_connect, ZBX_DB_TLS_CONNECT_VERIFY_CA_TXT) &&
			0 != strcmp(config_dbhigh->config_db_tls_connect, ZBX_DB_TLS_CONNECT_VERIFY_FULL_TXT))
	{
		zabbix_log(LOG_LEVEL_CRIT, "invalid \"DBTLSConnect\" configuration parameter: '%s'",
				config_dbhigh->config_db_tls_connect);
		exit(EXIT_FAILURE);
	}

	if (NULL != config_dbhigh->config_db_tls_connect &&
			(0 == strcmp(ZBX_DB_TLS_CONNECT_VERIFY_CA_TXT, config_dbhigh->config_db_tls_connect) ||
			0 == strcmp(ZBX_DB_TLS_CONNECT_VERIFY_FULL_TXT, config_dbhigh->config_db_tls_connect)) &&
			NULL == config_dbhigh->config_db_tls_ca_file)
	{
		zabbix_log(LOG_LEVEL_CRIT, "parameter \"DBTLSConnect\" value \"%s\" requires \"DBTLSCAFile\", but it"
				" is not defined", config_dbhigh->config_db_tls_connect);
		exit(EXIT_FAILURE);
	}

	if ((NULL != config_dbhigh->config_db_tls_cert_file || NULL != config_dbhigh->config_db_tls_key_file) &&
			(NULL == config_dbhigh->config_db_tls_cert_file ||
			NULL == config_dbhigh->config_db_tls_key_file || NULL == config_dbhigh->config_db_tls_ca_file))
	{
		zabbix_log(LOG_LEVEL_CRIT, "parameter \"DBTLSKeyFile\" or \"DBTLSCertFile\" is defined, but"
				" \"DBTLSKeyFile\", \"DBTLSCertFile\" or \"DBTLSCAFile\" is not defined");
		exit(EXIT_FAILURE);
	}
}
#endif

/******************************************************************************
 *                                                                            *
 * Purpose: specify the autoincrement options when connecting to the database *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_init_autoincrement_options(void)
{
	zbx_db_init_autoincrement_options_basic();
}

/******************************************************************************
 *                                                                            *
 * Purpose: connect to the database                                           *
 *                                                                            *
 * Parameters: flag - ZBX_DB_CONNECT_ONCE (try once and return the result),   *
 *                    ZBX_DB_CONNECT_EXIT (exit on failure) or                *
 *                    ZBX_DB_CONNECT_NORMAL (retry until connected)           *
 *                                                                            *
 * Return value: same as zbx_db_connect_basic()                               *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_connect(int flag)
{
	int	err;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s() flag:%d", __func__, flag);

	while (ZBX_DB_OK != (err = zbx_db_connect_basic(zbx_cfg_dbhigh)))
	{
		if (ZBX_DB_CONNECT_ONCE == flag)
			break;

		if (ZBX_DB_FAIL == err || ZBX_DB_CONNECT_EXIT == flag)
		{
			zabbix_log(LOG_LEVEL_CRIT, "Cannot connect to the database. Exiting...");
			exit(EXIT_FAILURE);
		}

		zabbix_log(LOG_LEVEL_ERR, "database is down: reconnecting in %d seconds", ZBX_DB_WAIT_DOWN);
		connection_failure = 1;
		zbx_sleep(ZBX_DB_WAIT_DOWN);
	}

	if (0 != connection_failure)
	{
		zabbix_log(LOG_LEVEL_ERR, "database connection re-established");
		connection_failure = 0;
	}

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():%d", __func__, err);

	return err;
}

int	zbx_db_init(zbx_dc_get_nextid_func_t cb_nextid, int log_slow_queries, char **error)
{
	zbx_cb_nextid = cb_nextid;

	return zbx_db_init_basic(zbx_cfg_dbhigh->config_dbname, zbx_dbschema_get_schema(), log_slow_queries, error);
}

void	zbx_db_deinit(void)
{
	zbx_db_deinit_basic();
}

/******************************************************************************
 *                                                                            *
 * Purpose: helper function to loop transaction operation while DB is down    *
 *                                                                            *
 ******************************************************************************/
static void	DBtxn_operation(int (*txn_operation)(void))
{
	int	rc;

	rc = txn_operation();

	while (ZBX_DB_DOWN == rc)
	{
		zbx_db_close();
		zbx_db_connect(ZBX_DB_CONNECT_NORMAL);

		if (ZBX_DB_DOWN == (rc = txn_operation()))
		{
			zabbix_log(LOG_LEVEL_ERR, "database is down: retrying in %d seconds", ZBX_DB_WAIT_DOWN);
			connection_failure = 1;
			sleep(ZBX_DB_WAIT_DOWN);
		}
	}
}

/******************************************************************************
 *                                                                            *
 * Purpose: start a transaction                                               *
 *                                                                            *
 * Comments: do nothing if DB does not support transactions                   *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_begin(void)
{
	DBtxn_operation(zbx_db_begin_basic);
}

/******************************************************************************
 *                                                                            *
 * Purpose: commit a transaction                                              *
 *                                                                            *
 * Comments: do nothing if DB does not support transactions                   *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_commit(void)
{
	if (ZBX_DB_OK > zbx_db_commit_basic())
	{
		zabbix_log(LOG_LEVEL_DEBUG, "commit called on failed transaction, doing a rollback instead");
		zbx_db_rollback();
	}

	return zbx_db_txn_end_error();
}

/******************************************************************************
 *                                                                            *
 * Purpose: rollback a transaction                                            *
 *                                                                            *
 * Comments: do nothing if DB does not support transactions                   *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_rollback(void)
{
	if (ZBX_DB_OK > zbx_db_rollback_basic())
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot perform transaction rollback, connection will be reset");

		zbx_db_close();
		zbx_db_connect(ZBX_DB_CONNECT_NORMAL);
	}
	else
	{
		if (ZBX_DB_DOWN == zbx_db_txn_end_error() && ERR_Z3009 == zbx_db_last_errcode())
		{
			zabbix_log(LOG_LEVEL_ERR, "database is read-only: waiting for %d seconds", ZBX_DB_WAIT_DOWN);
			sleep(ZBX_DB_WAIT_DOWN);
		}
	}
}

/******************************************************************************
 *                                                                            *
 * Purpose: commit or rollback a transaction depending on a parameter value   *
 *                                                                            *
 * Comments: do nothing if DB does not support transactions                   *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_end(int ret)
{
	if (SUCCEED == ret)
		return ZBX_DB_OK == zbx_db_commit() ? SUCCEED : FAIL;

	zbx_db_rollback();

	return FAIL;
}

/******************************************************************************
 *                                                                            *
 * Purpose: execute a non-select statement                                    *
 *                                                                            *
 * Comments: retry until DB is up                                             *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_execute(const char *fmt, ...)
{
	va_list	args;
	int	rc;

	va_start(args, fmt);

	rc = zbx_db_vexecute(fmt, args);

	while (ZBX_DB_DOWN == rc)
	{
		zbx_db_close();
		zbx_db_connect(ZBX_DB_CONNECT_NORMAL);

		if (ZBX_DB_DOWN == (rc = zbx_db_vexecute(fmt, args)))
		{
			zabbix_log(LOG_LEVEL_ERR, "database is down: retrying in %d seconds", ZBX_DB_WAIT_DOWN);
			connection_failure = 1;
			sleep(ZBX_DB_WAIT_DOWN);
		}
	}

	va_end(args);

	return rc;
}

/******************************************************************************
 *                                                                            *
 * Purpose: execute a non-select statement                                    *
 *                                                                            *
 * Comments: don't retry if DB is down                                        *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_execute_once(const char *fmt, ...)
{
	va_list	args;
	int	rc;

	va_start(args, fmt);

	rc = zbx_db_vexecute(fmt, args);

	va_end(args);

	return rc;
}

/******************************************************************************
 *                                                                            *
 * Purpose: check if numeric field value is null                              *
 *                                                                            *
 * Parameters: field - [IN] field value to be checked                         *
 *                                                                            *
 * Return value: SUCCEED - field value is null                                *
 *               FAIL    - otherwise                                          *
 *                                                                            *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_is_null(const char *field)
{
	return zbx_db_is_null_basic(field);
}

zbx_db_row_t	zbx_db_fetch(zbx_db_result_t result)
{
	return zbx_db_fetch_basic(result);
}

/******************************************************************************
 *                                                                            *
 * Purpose: execute a select statement                                        *
 *                                                                            *
 * Comments: don't retry until DB is up                                       *
 *                                                                            *
 ******************************************************************************/
zbx_db_result_t	zbx_db_select_once(const char *fmt, ...)
{
	va_list		args;
	zbx_db_result_t	rc;

	va_start(args, fmt);

	rc = zbx_db_vselect(fmt, args);

	va_end(args);

	return rc;
}

/******************************************************************************
 *                                                                            *
 * Purpose: execute a select statement                                        *
 *                                                                            *
 * Comments: retry until DB is up                                             *
 *                                                                            *
 ******************************************************************************/
zbx_db_result_t	zbx_db_select(const char *fmt, ...)
{
	va_list		args;
	zbx_db_result_t	rc;

	va_start(args, fmt);

	rc = zbx_db_vselect(fmt, args);

	while ((zbx_db_result_t)ZBX_DB_DOWN == rc)
	{
		zbx_db_close();
		zbx_db_connect(ZBX_DB_CONNECT_NORMAL);

		if ((zbx_db_result_t)ZBX_DB_DOWN == (rc = zbx_db_vselect(fmt, args)))
		{
			zabbix_log(LOG_LEVEL_ERR, "database is down: retrying in %d seconds", ZBX_DB_WAIT_DOWN);
			connection_failure = 1;
			sleep(ZBX_DB_WAIT_DOWN);
		}
	}

	va_end(args);

	return rc;
}

/******************************************************************************
 *                                                                            *
 * Purpose: execute a select statement and get the first N entries            *
 *                                                                            *
 * Comments: retry until DB is up                                             *
 *                                                                            *
 ******************************************************************************/
zbx_db_result_t	zbx_db_select_n(const char *query, int n)
{
	zbx_db_result_t	rc;

	rc = zbx_db_select_n_basic(query, n);

	while ((zbx_db_result_t)ZBX_DB_DOWN == rc)
	{
		zbx_db_close();
		zbx_db_connect(ZBX_DB_CONNECT_NORMAL);

		if ((zbx_db_result_t)ZBX_DB_DOWN == (rc = zbx_db_select_n_basic(query, n)))
		{
			zabbix_log(LOG_LEVEL_ERR, "database is down: retrying in %d seconds", ZBX_DB_WAIT_DOWN);
			connection_failure = 1;
			sleep(ZBX_DB_WAIT_DOWN);
		}
	}

	return rc;
}

#ifdef HAVE_MYSQL
static size_t	get_string_field_size(const zbx_db_field_t *field)
{
	switch (field->type)
	{
		case ZBX_TYPE_BLOB:
		case ZBX_TYPE_LONGTEXT:
			return 4294967295ul;
		case ZBX_TYPE_CHAR:
		case ZBX_TYPE_TEXT:
			return 65535u;
		case ZBX_TYPE_CUID:
			return CUID_LEN - 1;
		default:
			THIS_SHOULD_NEVER_HAPPEN;
			exit(EXIT_FAILURE);
	}
}
#endif

static size_t	get_string_field_chars(const zbx_db_field_t *field)
{
	if ((ZBX_TYPE_LONGTEXT == field->type || ZBX_TYPE_BLOB == field->type) && 0 == field->length)
		return ZBX_SIZE_T_MAX;
	else if (ZBX_TYPE_CUID == field->type)
		return CUID_LEN - 1;
	else
		return field->length;
}

char	*zbx_db_dyn_escape_string_len(const char *src, size_t length)
{
	return zbx_db_dyn_escape_string_basic(src, ZBX_SIZE_T_MAX, length, ESCAPE_SEQUENCE_ON);
}

char	*zbx_db_dyn_escape_string(const char *src)
{
	return zbx_db_dyn_escape_string_basic(src, ZBX_SIZE_T_MAX, ZBX_SIZE_T_MAX, ESCAPE_SEQUENCE_ON);
}

static char	*DBdyn_escape_field_len(const zbx_db_field_t *field, const char *src, zbx_escape_sequence_t flag)
{
#if defined(HAVE_MYSQL)
	return zbx_db_dyn_escape_string_basic(src, get_string_field_size(field), get_string_field_chars(field), flag);
#else
	return zbx_db_dyn_escape_string_basic(src, ZBX_SIZE_T_MAX, get_string_field_chars(field), flag);
#endif
}

char	*zbx_db_dyn_escape_field(const char *table_name, const char *field_name, const char *src)
{
	const zbx_db_table_t	*table;
	const zbx_db_field_t	*field;

	if (NULL == (table = zbx_db_get_table(table_name)) || NULL == (field = zbx_db_get_field(table, field_name)))
	{
		zabbix_log(LOG_LEVEL_CRIT, "invalid table: \"%s\" field: \"%s\"", table_name, field_name);
		exit(EXIT_FAILURE);
	}

	return DBdyn_escape_field_len(field, src, ESCAPE_SEQUENCE_ON);
}

char	*zbx_db_dyn_escape_like_pattern(const char *src)
{
	return zbx_db_dyn_escape_like_pattern_basic(src);
}

static zbx_db_table_t	*db_get_table(const char *tablename)
{
	zbx_db_table_t	*tables = zbx_dbschema_get_tables();

	for (int t = 0; NULL != tables[t].table; t++)
	{
		if (0 == strcmp(tables[t].table, tablename))
			return &tables[t];
	}

	return NULL;
}

static zbx_db_field_t	*db_get_field(zbx_db_table_t *table, const char *fieldname)
{
	int	f;

	for (f = 0; NULL != table->fields[f].name; f++)
	{
		if (0 == strcmp(table->fields[f].name, fieldname))
			return &table->fields[f];
	}

	return NULL;
}

const zbx_db_table_t	*zbx_db_get_table(const char *tablename)
{
	return db_get_table(tablename);
}

const zbx_db_field_t	*zbx_db_get_field(const zbx_db_table_t *table, const char *fieldname)
{
	return db_get_field((zbx_db_table_t *)table, fieldname);
}

int	zbx_db_validate_field_size(const char *tablename, const char *fieldname, const char *str)
{
	const zbx_db_table_t	*table;
	const zbx_db_field_t	*field;
	size_t			max_bytes, max_chars;

	if (NULL == (table = zbx_db_get_table(tablename)) || NULL == (field = zbx_db_get_field(table, fieldname)))
	{
		zabbix_log(LOG_LEVEL_CRIT, "invalid table: \"%s\" field: \"%s\"", tablename, fieldname);
		return FAIL;
	}

#if defined(HAVE_MYSQL)
	max_bytes = get_string_field_size(field);
#else
	max_bytes = ZBX_SIZE_T_MAX;
#endif
	max_chars = get_string_field_chars(field);

	if (max_bytes < strlen(str))
		return FAIL;

	if (ZBX_SIZE_T_MAX == max_chars)
		return SUCCEED;

	if (max_chars != max_bytes && max_chars < zbx_strlen_utf8(str))
		return FAIL;

	return SUCCEED;
}

/******************************************************************************
 *                                                                            *
 * Purpose: gets a new identifier(s) for a specified table                    *
 *                                                                            *
 * Parameters: tablename - [IN] the name of a table                           *
 *             num       - [IN] the number of reserved records                *
 *                                                                            *
 * Return value: first reserved identifier                                    *
 *                                                                            *
 ******************************************************************************/
static zbx_uint64_t	DBget_nextid(const char *tablename, int num)
{
	zbx_db_result_t		result;
	zbx_db_row_t		row;
	zbx_uint64_t		ret1, ret2;
	zbx_uint64_t		min = 0, max = ZBX_DB_MAX_ID;
	int			found = FAIL, dbres;
	const zbx_db_table_t	*table;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s() tablename:'%s'", __func__, tablename);

	if (NULL == (table = zbx_db_get_table(tablename)))
	{
		zabbix_log(LOG_LEVEL_CRIT, "Error getting table: %s", tablename);
		THIS_SHOULD_NEVER_HAPPEN;
		exit(EXIT_FAILURE);
	}

	while (FAIL == found)
	{
		/* avoid eternal loop within failed transaction */
		if (0 < zbx_db_txn_level() && 0 != zbx_db_txn_error())
		{
			zabbix_log(LOG_LEVEL_DEBUG, "End of %s() transaction failed", __func__);
			return 0;
		}

		result = zbx_db_select("select nextid from ids where table_name='%s' and field_name='%s'",
				table->table, table->recid);

		if (NULL == (row = zbx_db_fetch(result)))
		{
			zbx_db_free_result(result);

			result = zbx_db_select("select max(%s) from %s where %s between " ZBX_FS_UI64 " and " ZBX_FS_UI64,
					table->recid, table->table, table->recid, min, max);

			if (NULL == (row = zbx_db_fetch(result)) || SUCCEED == zbx_db_is_null(row[0]))
			{
				ret1 = min;
			}
			else
			{
				ZBX_STR2UINT64(ret1, row[0]);
				if (ret1 >= max)
				{
					zabbix_log(LOG_LEVEL_CRIT, "maximum number of id's exceeded"
							" [table:%s, field:%s, id:" ZBX_FS_UI64 "]",
							table->table, table->recid, ret1);
					exit(EXIT_FAILURE);
				}
			}

			zbx_db_free_result(result);

			dbres = zbx_db_execute("insert into ids (table_name,field_name,nextid)"
					" values ('%s','%s'," ZBX_FS_UI64 ")",
					table->table, table->recid, ret1);

			if (ZBX_DB_OK > dbres)
			{
				/* solving the problem of an invisible record created in a parallel transaction */
				zbx_db_execute("update ids set nextid=nextid+1 where table_name='%s' and field_name='%s'",
						table->table, table->recid);
			}

			continue;
		}
		else
		{
			ZBX_STR2UINT64(ret1, row[0]);
			zbx_db_free_result(result);

			if (ret1 < min || ret1 >= max)
			{
				zbx_db_execute("delete from ids where table_name='%s' and field_name='%s'",
						table->table, table->recid);
				continue;
			}

			zbx_db_execute("update ids set nextid=nextid+%d where table_name='%s' and field_name='%s'",
					num, table->table, table->recid);

			result = zbx_db_select("select nextid from ids where table_name='%s' and field_name='%s'",
					table->table, table->recid);

			if (NULL != (row = zbx_db_fetch(result)) && SUCCEED != zbx_db_is_null(row[0]))
			{
				ZBX_STR2UINT64(ret2, row[0]);

				if (ret1 + num == ret2)
					found = SUCCEED;
			}
			else
				THIS_SHOULD_NEVER_HAPPEN;

			zbx_db_free_result(result);
		}
	}

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():" ZBX_FS_UI64 " table:'%s' recid:'%s'",
			__func__, ret2 - num + 1, table->table, table->recid);

	return ret2 - num + 1;
}

zbx_uint64_t	zbx_db_get_maxid_num(const char *tablename, int num)
{
	if (0 == strcmp(tablename, "events") ||
			0 == strcmp(tablename, "event_tag") ||
			0 == strcmp(tablename, "problem_tag") ||
			0 == strcmp(tablename, "dservices") ||
			0 == strcmp(tablename, "dhosts") ||
			0 == strcmp(tablename, "alerts") ||
			0 == strcmp(tablename, "escalations") ||
			0 == strcmp(tablename, "autoreg_host") ||
			0 == strcmp(tablename, "event_suppress") ||
			0 == strcmp(tablename, "trigger_queue") ||
			0 == strcmp(tablename, "proxy_history") ||
			0 == strcmp(tablename, "proxy_dhistory") ||
			0 == strcmp(tablename, "proxy_autoreg_host") ||
			0 == strcmp(tablename, "host_proxy"))
		return zbx_cb_nextid(tablename, num);

	return DBget_nextid(tablename, num);
}

/******************************************************************************
 *                                                                            *
 * Purpose: connects to DB and tries to detect DB version                     *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_extract_version_info(struct zbx_db_version_info_t *version_info)
{
	zbx_dbms_version_info_extract(version_info);
}

#ifdef HAVE_POSTGRESQL
/******************************************************************************
 *                                                                            *
 * Purpose: retrieves TimescaleDB (TSDB) license information                  *
 *                                                                            *
 * Return value: license information from datase as string                    *
 *               "apache"    for TimescaleDB Apache 2 Edition                 *
 *               "timescale" for TimescaleDB Community Edition                *
 *                                                                            *
 * Comments: returns a pointer to allocated memory                            *
 *                                                                            *
 ******************************************************************************/
static char	*zbx_tsdb_get_license(void)
{
	zbx_db_result_t	result;
	zbx_db_row_t	row;
	char		*tsdb_lic = NULL;

	result = zbx_db_select("show timescaledb.license");

	if ((zbx_db_result_t)ZBX_DB_DOWN != result && NULL != result && NULL != (row = zbx_db_fetch(result)))
	{
		tsdb_lic = zbx_strdup(NULL, row[0]);
	}

	zbx_db_free_result(result);

	return tsdb_lic;
}
#endif

/*********************************************************************************
 *                                                                               *
 * Purpose: verify that Zabbix can work with DB extension                        *
 *                                                                               *
 * Parameters: info              - [IN] DB version information                   *
 *             allow_unsupported - [IN] value of AllowUnsupportedDBVersions flag *
 *                                                                               *
 * Return value: SUCCEED if the operation completed successfully or              *
 *               FAIL otherwise.                                                 *
 *********************************************************************************/
int	zbx_db_check_extension(struct zbx_db_version_info_t *info, int allow_unsupported)
{
#ifdef HAVE_POSTGRESQL
	zbx_db_result_t	result;
	zbx_db_row_t	row;
	char		*tsdb_lic = NULL;
	int		ret = SUCCEED;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __func__);

	/* in case of major upgrade, db_extension may be missing */
	if (FAIL == zbx_db_field_exists("config", "db_extension"))
		goto out;

	if (NULL == (result = zbx_db_select("select db_extension from config")))
		goto out;

	if (NULL == (row = zbx_db_fetch(result)) || '\0' == *row[0])
	{
		zbx_db_free_result(result);
		goto out;
	}

	info->extension = zbx_strdup(NULL, row[0]);

	zbx_db_free_result(result);

	if (0 != zbx_strcmp_null(info->extension, ZBX_DB_EXTENSION_TIMESCALEDB))
		goto out;

	/* at this point we know the TimescaleDB extension is enabled in Zabbix */

	zbx_tsdb_info_extract(info);

	if (DB_VERSION_FAILED_TO_RETRIEVE == info->ext_flag)
	{
		info->ext_err_code = ZBX_TIMESCALEDB_VERSION_FAILED_TO_RETRIEVE;
		ret = FAIL;
		goto out;
	}

	if (DB_VERSION_LOWER_THAN_MINIMUM == info->ext_flag)
	{
		zabbix_log(LOG_LEVEL_WARNING, "TimescaleDB version must be at least %d. Recommended version is at least"
				" %s %s.", ZBX_TIMESCALE_MIN_VERSION, ZBX_TIMESCALE_LICENSE_COMMUNITY_STR,
				ZBX_TIMESCALE_MIN_SUPPORTED_VERSION_STR);
		info->ext_err_code = ZBX_TIMESCALEDB_VERSION_LOWER_THAN_MINIMUM;
		ret = FAIL;
		goto out;
	}

	if (DB_VERSION_NOT_SUPPORTED_ERROR == info->ext_flag)
	{
		zabbix_log(LOG_LEVEL_WARNING, "TimescaleDB version %u is not officially supported. Recommended version"
				" is at least %s %s.", info->ext_current_version,
				ZBX_TIMESCALE_LICENSE_COMMUNITY_STR, ZBX_TIMESCALE_MIN_SUPPORTED_VERSION_STR);
		info->ext_err_code = ZBX_TIMESCALEDB_VERSION_NOT_SUPPORTED;

		if (0 == allow_unsupported)
		{
			ret = FAIL;
			goto out;
		}

		info->ext_flag = DB_VERSION_NOT_SUPPORTED_WARNING;
	}

	if (DB_VERSION_HIGHER_THAN_MAXIMUM == info->ext_flag)
	{
		zabbix_log(LOG_LEVEL_WARNING, "TimescaleDB version is too new. Recommended version is up to %s %s.",
				ZBX_TIMESCALE_LICENSE_COMMUNITY_STR, ZBX_TIMESCALE_MAX_VERSION_STR);
		info->ext_err_code = ZBX_TIMESCALEDB_VERSION_HIGHER_THAN_MAXIMUM;

		if (0 == allow_unsupported)
		{
			info->ext_flag = DB_VERSION_HIGHER_THAN_MAXIMUM_ERROR;
			ret = FAIL;
			goto out;
		}

		info->ext_flag = DB_VERSION_HIGHER_THAN_MAXIMUM_WARNING;
	}

	tsdb_lic = zbx_tsdb_get_license();

	zbx_tsdb_extract_compressed_chunk_flags(info);

	zabbix_log(LOG_LEVEL_DEBUG, "TimescaleDB license: [%s]", ZBX_NULL2EMPTY_STR(tsdb_lic));

	if (0 != zbx_strcmp_null(tsdb_lic, ZBX_TIMESCALE_LICENSE_COMMUNITY))
	{
		zabbix_log(LOG_LEVEL_WARNING, "Detected license [%s] does not support compression. Compression is"
				" supported in %s.", ZBX_NULL2EMPTY_STR(tsdb_lic),
				ZBX_TIMESCALE_LICENSE_COMMUNITY_STR);
		info->ext_err_code = ZBX_TIMESCALEDB_LICENSE_NOT_COMMUNITY;
		goto out;
	}

	zabbix_log(LOG_LEVEL_DEBUG, "%s was detected. TimescaleDB compression is supported.",
			ZBX_TIMESCALE_LICENSE_COMMUNITY_STR);

	if (ZBX_EXT_ERR_UNDEFINED == info->ext_err_code)
		info->ext_err_code = ZBX_EXT_SUCCEED;

	zbx_tsdb_set_compression_availability(ON);
out:
	zbx_free(tsdb_lic);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():%s", __func__, zbx_result_string(ret));

	return ret;
#else
	ZBX_UNUSED(info);
	ZBX_UNUSED(allow_unsupported);

	return SUCCEED;
#endif
}

/******************************************************************************
 *                                                                            *
 * Purpose: writes a json entry in DB with the result for the front-end       *
 *                                                                            *
 * Parameters: version - [IN] entry of DB versions                            *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_flush_version_requirements(const char *version)
{
	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __func__);

	if (ZBX_DB_OK > zbx_db_execute("update config set dbversion_status='%s'", version))
		zabbix_log(LOG_LEVEL_CRIT, "Failed to set dbversion_status");

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __func__);
}

/*********************************************************************************
 *                                                                               *
 * Purpose: verify that Zabbix server/proxy will start with provided DB version  *
 *          and configuration                                                    *
 *                                                                               *
 * Parameters: info              - [IN] DB version information                   *
 *             allow_unsupported - [IN] value of AllowUnsupportedDBVersions flag *
 *             program_type      - [IN]                                          *
 *                                                                               *
 *********************************************************************************/
int	zbx_db_check_version_info(struct zbx_db_version_info_t *info, int allow_unsupported,
		unsigned char program_type)
{
	zbx_db_extract_version_info(info);

	if (DB_VERSION_NOT_SUPPORTED_ERROR == info->flag ||
			DB_VERSION_HIGHER_THAN_MAXIMUM == info->flag || DB_VERSION_LOWER_THAN_MINIMUM == info->flag)
	{
		const char	*program_type_s;
		int		server_db_deprecated;

		program_type_s = get_program_type_string(program_type);

		server_db_deprecated = (DB_VERSION_LOWER_THAN_MINIMUM == info->flag &&
				0 != (program_type & ZBX_PROGRAM_TYPE_SERVER));

		if (0 == allow_unsupported || 0 != server_db_deprecated)
		{
			zabbix_log(LOG_LEVEL_ERR, " ");
			zabbix_log(LOG_LEVEL_ERR, "Unable to start Zabbix %s due to unsupported %s database"
					" version (%s).", program_type_s, info->database,
					info->friendly_current_version);

			if (DB_VERSION_HIGHER_THAN_MAXIMUM == info->flag)
			{
				zabbix_log(LOG_LEVEL_ERR, "Must not be higher than (%s).",
						info->friendly_max_version);
				info->flag = DB_VERSION_HIGHER_THAN_MAXIMUM_ERROR;
			}
			else
			{
				zabbix_log(LOG_LEVEL_ERR, "Must be at least (%s).",
						info->friendly_min_supported_version);
			}

			zabbix_log(LOG_LEVEL_ERR, "Use of supported database version is highly recommended.");

			if (0 == server_db_deprecated)
			{
				zabbix_log(LOG_LEVEL_ERR, "Override by setting AllowUnsupportedDBVersions=1"
						" in Zabbix %s configuration file at your own risk.", program_type_s);
			}

			zabbix_log(LOG_LEVEL_ERR, " ");

			return FAIL;
		}
		else
		{
			zabbix_log(LOG_LEVEL_ERR, " ");
			zabbix_log(LOG_LEVEL_ERR, "Warning! Unsupported %s database version (%s).",
					info->database, info->friendly_current_version);

			if (DB_VERSION_HIGHER_THAN_MAXIMUM == info->flag)
			{
				zabbix_log(LOG_LEVEL_ERR, "Should not be higher than (%s).",
						info->friendly_max_version);
				info->flag = DB_VERSION_HIGHER_THAN_MAXIMUM_WARNING;
			}
			else
			{
				zabbix_log(LOG_LEVEL_ERR, "Should be at least (%s).",
						info->friendly_min_supported_version);
				info->flag = DB_VERSION_NOT_SUPPORTED_WARNING;
			}

			zabbix_log(LOG_LEVEL_ERR, "Use of supported database version is highly recommended.");
			zabbix_log(LOG_LEVEL_ERR, " ");
		}
	}

	return SUCCEED;
}

void	zbx_db_version_info_clear(struct zbx_db_version_info_t *version_info)
{
	zbx_free(version_info->friendly_current_version);
	zbx_free(version_info->extension);
	zbx_free(version_info->ext_friendly_current_version);
}

#if defined(HAVE_MYSQL) || defined(HAVE_POSTGRESQL)
#define MAX_EXPRESSIONS	1000	/* tune according to batch size to avoid unnecessary or conditions */
#else
#define MAX_EXPRESSIONS	950
#endif

/******************************************************************************
 *                                                                            *
 * Purpose: Takes an initial part of SQL query and appends a generated        *
 *          WHERE condition. The WHERE condition is generated from the given  *
 *          list of values as a mix of <fieldname> BETWEEN <id1> AND <idN>"   *
 *          and "<fieldname> IN (<id1>,<id2>,...,<idN>)" elements.            *
 *                                                                            *
 * Parameters: sql        - [IN/OUT] buffer for SQL query construction        *
 *             sql_alloc  - [IN/OUT] size of the 'sql' buffer                 *
 *             sql_offset - [IN/OUT] current position in the 'sql' buffer     *
 *             fieldname  - [IN] field name to be used in SQL WHERE condition *
 *             values     - [IN] array of numerical values sorted in          *
 *                               ascending order to be included in WHERE      *
 *             num        - [IN] number of elements in 'values' array         *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_add_condition_alloc(char **sql, size_t *sql_alloc, size_t *sql_offset, const char *fieldname,
		const zbx_uint64_t *values, const int num)
{
	int		i, in_cnt;
#if defined(HAVE_SQLITE3)
	int		expr_num, expr_cnt = 0;
#endif
	if (0 == num)
		return;

	zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, ' ');
	if (MAX_EXPRESSIONS < num)
		zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, '(');

#if defined(HAVE_SQLITE3)
	expr_num = (num + MAX_EXPRESSIONS - 1) / MAX_EXPRESSIONS;

	if (MAX_EXPRESSIONS < expr_num)
		zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, '(');
#endif

	if (1 < num)
		zbx_snprintf_alloc(sql, sql_alloc, sql_offset, "%s in (", fieldname);

	/* compose "in"s */
	for (i = 0, in_cnt = 0; i < num; i++)
	{
		if (1 == num)
		{
			zbx_snprintf_alloc(sql, sql_alloc, sql_offset, "%s=" ZBX_FS_UI64, fieldname,
					values[i]);
			break;
		}
		else
		{
			if (MAX_EXPRESSIONS == in_cnt)
			{
				in_cnt = 0;
				(*sql_offset)--;
#if defined(HAVE_SQLITE3)
				if (MAX_EXPRESSIONS == ++expr_cnt)
				{
					zbx_snprintf_alloc(sql, sql_alloc, sql_offset, ")) or (%s in (",
							fieldname);
					expr_cnt = 0;
				}
				else
				{
#endif
					zbx_snprintf_alloc(sql, sql_alloc, sql_offset, ") or %s in (",
							fieldname);
#if defined(HAVE_SQLITE3)
				}
#endif
			}

			in_cnt++;
			zbx_snprintf_alloc(sql, sql_alloc, sql_offset, ZBX_FS_UI64 ",",
					values[i]);
		}
	}

	if (1 < num)
	{
		(*sql_offset)--;
		zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, ')');
	}

#if defined(HAVE_SQLITE3)
	if (MAX_EXPRESSIONS < expr_num)
		zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, ')');
#endif
	if (MAX_EXPRESSIONS < num)
		zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, ')');

#undef MAX_EXPRESSIONS
}

/*********************************************************************************
 *                                                                               *
 * Purpose: This function is similar to the zbx_db_add_condition_alloc(), except *
 *          it is designed for generating WHERE conditions for strings. Hence,   *
 *          this function is simpler, because only IN condition is possible.     *
 *                                                                               *
 * Parameters: sql        - [IN/OUT] buffer for SQL query construction           *
 *             sql_alloc  - [IN/OUT] size of the 'sql' buffer                    *
 *             sql_offset - [IN/OUT] current position in the 'sql' buffer        *
 *             fieldname  - [IN] field name to be used in SQL WHERE condition    *
 *             values     - [IN] array of string values                          *
 *             num        - [IN] number of elements in 'values' array            *
 *                                                                               *
 *                                                                               *
 *********************************************************************************/
void	zbx_db_add_str_condition_alloc(char **sql, size_t *sql_alloc, size_t *sql_offset, const char *fieldname,
		const char * const *values, const int num)
{
#if defined(HAVE_MYSQL) || defined(HAVE_POSTGRESQL)
#define MAX_EXPRESSIONS	1000	/* tune according to batch size to avoid unnecessary or conditions */
#else
#define MAX_EXPRESSIONS	950
#endif

	int	i, cnt = 0;
	char	*value_esc;
	int	values_num = 0, empty_num = 0;

	if (0 == num)
		return;

	zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, ' ');

	for (i = 0; i < num; i++)
	{
		if ('\0' == *values[i])
			empty_num++;
		else
			values_num++;
	}

	if (MAX_EXPRESSIONS < values_num || (0 != values_num && 0 != empty_num))
		zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, '(');

	if (0 != empty_num)
	{
		zbx_snprintf_alloc(sql, sql_alloc, sql_offset, "%s" ZBX_SQL_STRCMP, fieldname, ZBX_SQL_STRVAL_EQ(""));

		if (0 == values_num)
			return;

		zbx_strcpy_alloc(sql, sql_alloc, sql_offset, " or ");
	}

	if (1 == values_num)
	{
		for (i = 0; i < num; i++)
		{
			if ('\0' == *values[i])
				continue;

			value_esc = zbx_db_dyn_escape_string(values[i]);
			zbx_snprintf_alloc(sql, sql_alloc, sql_offset, "%s='%s'", fieldname, value_esc);
			zbx_free(value_esc);
		}

		if (0 != empty_num)
			zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, ')');
		return;
	}

	zbx_strcpy_alloc(sql, sql_alloc, sql_offset, fieldname);
	zbx_strcpy_alloc(sql, sql_alloc, sql_offset, " in (");

	for (i = 0; i < num; i++)
	{
		if ('\0' == *values[i])
			continue;

		if (MAX_EXPRESSIONS == cnt)
		{
			cnt = 0;
			(*sql_offset)--;
			zbx_strcpy_alloc(sql, sql_alloc, sql_offset, ") or ");
			zbx_strcpy_alloc(sql, sql_alloc, sql_offset, fieldname);
			zbx_strcpy_alloc(sql, sql_alloc, sql_offset, " in (");
		}

		value_esc = zbx_db_dyn_escape_string(values[i]);
		zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, '\'');
		zbx_strcpy_alloc(sql, sql_alloc, sql_offset, value_esc);
		zbx_strcpy_alloc(sql, sql_alloc, sql_offset, "',");
		zbx_free(value_esc);

		cnt++;
	}

	(*sql_offset)--;
	zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, ')');

	if (MAX_EXPRESSIONS < values_num || 0 != empty_num)
		zbx_chrcpy_alloc(sql, sql_alloc, sql_offset, ')');

#undef MAX_EXPRESSIONS
}

static char	buf_string[640];

/******************************************************************************
 *                                                                            *
 * Return value: <host> or "???" if host not found                            *
 *                                                                            *
 ******************************************************************************/
const char	*zbx_host_string(zbx_uint64_t hostid)
{
	zbx_db_result_t	result;
	zbx_db_row_t	row;

	result = zbx_db_select(
			"select host"
			" from hosts"
			" where hostid=" ZBX_FS_UI64,
			hostid);

	if (NULL != (row = zbx_db_fetch(result)))
		zbx_snprintf(buf_string, sizeof(buf_string), "%s", row[0]);
	else
		zbx_snprintf(buf_string, sizeof(buf_string), "???");

	zbx_db_free_result(result);

	return buf_string;
}

/******************************************************************************
 *                                                                            *
 * Return value: <host>:<key> or "???" if item not found                      *
 *                                                                            *
 ******************************************************************************/
const char	*zbx_host_key_string(zbx_uint64_t itemid)
{
	zbx_db_result_t	result;
	zbx_db_row_t	row;

	result = zbx_db_select(
			"select h.host,i.key_"
			" from hosts h,items i"
			" where h.hostid=i.hostid"
				" and i.itemid=" ZBX_FS_UI64,
			itemid);

	if (NULL != (row = zbx_db_fetch(result)))
		zbx_snprintf(buf_string, sizeof(buf_string), "%s:%s", row[0], row[1]);
	else
		zbx_snprintf(buf_string, sizeof(buf_string), "???");

	zbx_db_free_result(result);

	return buf_string;
}

/******************************************************************************
 *                                                                            *
 * Purpose: check if user has access rights to information - full name,       *
 *          alias, Email, SMS, etc                                            *
 *                                                                            *
 * Parameters: userid           - [IN] user who owns the information          *
 *             recipient_userid - [IN] user who will receive the information  *
 *                                     can be NULL for remote command         *
 *                                                                            *
 * Return value: SUCCEED - if information receiving user has access rights    *
 *               FAIL    - otherwise                                          *
 *                                                                            *
 * Comments: Users has access rights or can view personal information only    *
 *           about themselves and other user who belong to their group.       *
 *           "Zabbix Super Admin" can view and has access rights to           *
 *           information about any user.                                      *
 *                                                                            *
 ******************************************************************************/
int	zbx_check_user_permissions(const zbx_uint64_t *userid, const zbx_uint64_t *recipient_userid)
{
	zbx_db_result_t	result;
	zbx_db_row_t	row;
	int		user_type = -1, ret = SUCCEED;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __func__);

	if (NULL == recipient_userid || *userid == *recipient_userid)
		goto out;

	result = zbx_db_select("select r.type from users u,role r where u.roleid=r.roleid and"
			" userid=" ZBX_FS_UI64, *recipient_userid);

	if (NULL != (row = zbx_db_fetch(result)) && FAIL == zbx_db_is_null(row[0]))
		user_type = atoi(row[0]);
	zbx_db_free_result(result);

	if (-1 == user_type)
	{
		zabbix_log(LOG_LEVEL_DEBUG, "%s() cannot check permissions", __func__);
		ret = FAIL;
		goto out;
	}

	if (USER_TYPE_SUPER_ADMIN != user_type)
	{
		/* check if users are from the same user group */
		result = zbx_db_select(
				"select null"
				" from users_groups ug1"
				" where ug1.userid=" ZBX_FS_UI64
					" and exists (select null"
						" from users_groups ug2"
						" where ug1.usrgrpid=ug2.usrgrpid"
							" and ug2.userid=" ZBX_FS_UI64
					")",
				*userid, *recipient_userid);

		if (NULL == zbx_db_fetch(result))
			ret = FAIL;
		zbx_db_free_result(result);
	}
out:
	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():%s", __func__, zbx_result_string(ret));

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Return value: "Name Surname (Alias)" or "unknown" if user not found        *
 *                                                                            *
 ******************************************************************************/
const char	*zbx_user_string(zbx_uint64_t userid)
{
	zbx_db_result_t	result;
	zbx_db_row_t	row;

	result = zbx_db_select("select name,surname,username from users where userid=" ZBX_FS_UI64, userid);

	if (NULL != (row = zbx_db_fetch(result)))
		zbx_snprintf(buf_string, sizeof(buf_string), "%s %s (%s)", row[0], row[1], row[2]);
	else
		zbx_snprintf(buf_string, sizeof(buf_string), "unknown");

	zbx_db_free_result(result);

	return buf_string;
}

/******************************************************************************
 *                                                                            *
 * Purpose: get user username, name and surname                               *
 *                                                                            *
 * Parameters: userid     - [IN] user id                                      *
 *             username   - [OUT] user alias                                  *
 *             name       - [OUT] user name                                   *
 *             surname    - [OUT] user surname                                *
 *                                                                            *
 * Return value: SUCCEED or FAIL                                              *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_get_user_names(zbx_uint64_t userid, char **username, char **name, char **surname)
{
	int		ret = FAIL;
	zbx_db_result_t	result;
	zbx_db_row_t	row;

	if (NULL == (result = zbx_db_select(
			"select username,name,surname"
			" from users"
			" where userid=" ZBX_FS_UI64, userid)))
	{
		goto out;
	}

	if (NULL == (row = zbx_db_fetch(result)))
		goto out;

	*username = zbx_strdup(NULL, row[0]);
	*name = zbx_strdup(NULL, row[1]);
	*surname = zbx_strdup(NULL, row[2]);

	ret = SUCCEED;
out:
	zbx_db_free_result(result);

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: construct where condition                                         *
 *                                                                            *
 * Return value: "=<id>" if id not equal zero,                                *
 *               otherwise " is null"                                         *
 *                                                                            *
 * Comments: NB! Do not use this function more than once in same SQL query    *
 *                                                                            *
 ******************************************************************************/
const char	*zbx_db_sql_id_cmp(zbx_uint64_t id)
{
	static char		buf[22];	/* 1 - '=', 20 - value size, 1 - '\0' */
	static const char	is_null[9] = " is null";

	if (0 == id)
		return is_null;

	zbx_snprintf(buf, sizeof(buf), "=" ZBX_FS_UI64, id);

	return buf;
}

/******************************************************************************
 *                                                                            *
 * Purpose: flush SQL request                                                 *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_flush_overflowed_sql(char *sql, size_t sql_offset)
{
	if (0 != sql_offset)
		return zbx_db_execute("%s", sql);

	return ZBX_DB_OK;
}

/******************************************************************************
 *                                                                            *
 * Purpose: execute a set of SQL statements IF it is big enough               *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_execute_overflowed_sql(char **sql, size_t *sql_alloc, size_t *sql_offset)
{
	int	ret = SUCCEED;

	if (ZBX_MAX_OVERFLOW_SQL_SIZE < *sql_offset)
	{
#ifdef HAVE_MULTIROW_INSERT
		if (',' == (*sql)[*sql_offset - 1])
		{
			(*sql_offset)--;
			zbx_strcpy_alloc(sql, sql_alloc, sql_offset, ";\n");
		}
#else
		ZBX_UNUSED(sql_alloc);
#endif
		if (ZBX_DB_OK > zbx_db_execute("%s", *sql))
			ret = FAIL;

		*sql_offset = 0;

	}

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: construct a unique host name by the given sample                  *
 *                                                                            *
 * Parameters: host_name_sample - a host name to start constructing from      *
 *             field_name       - field name for host or host visible name    *
 *                                                                            *
 * Return value: unique host name which does not exist in the database        *
 *                                                                            *
 * Comments: the sample cannot be empty                                       *
 *           constructs new by adding "_$(number+1)", where "number"          *
 *           shows count of the sample itself plus already constructed ones   *
 *           host_name_sample is not modified, allocates new memory!          *
 *                                                                            *
 ******************************************************************************/
char	*zbx_db_get_unique_hostname_by_sample(const char *host_name_sample, const char *field_name)
{
	zbx_db_result_t		result;
	zbx_db_row_t		row;
	int			full_match = 0, i;
	char			*host_name_temp = NULL, *host_name_sample_esc;
	zbx_vector_uint64_t	nums;
	zbx_uint64_t		num = 2;	/* produce alternatives starting from "2" */
	size_t			sz;

	assert(host_name_sample && *host_name_sample);

	zabbix_log(LOG_LEVEL_DEBUG, "In %s() sample:'%s'", __func__, host_name_sample);

	zbx_vector_uint64_create(&nums);
	zbx_vector_uint64_reserve(&nums, 8);

	sz = strlen(host_name_sample);
	host_name_sample_esc = zbx_db_dyn_escape_like_pattern(host_name_sample);

	result = zbx_db_select(
			"select %s"
			" from hosts"
			" where %s like '%s%%' escape '%c'"
				" and flags<>%d"
				" and status in (%d,%d,%d)",
				field_name, field_name, host_name_sample_esc, ZBX_SQL_LIKE_ESCAPE_CHAR,
			ZBX_FLAG_DISCOVERY_PROTOTYPE,
			HOST_STATUS_MONITORED, HOST_STATUS_NOT_MONITORED, HOST_STATUS_TEMPLATE);

	zbx_free(host_name_sample_esc);

	while (NULL != (row = zbx_db_fetch(result)))
	{
		zbx_uint64_t	n;
		const char	*p;

		if (0 != strncmp(row[0], host_name_sample, sz))
			continue;

		p = row[0] + sz;

		if ('\0' == *p)
		{
			full_match = 1;
			continue;
		}

		if ('_' != *p || FAIL == zbx_is_uint64(p + 1, &n))
			continue;

		zbx_vector_uint64_append(&nums, n);
	}
	zbx_db_free_result(result);

	zbx_vector_uint64_sort(&nums, ZBX_DEFAULT_UINT64_COMPARE_FUNC);

	if (0 == full_match)
	{
		host_name_temp = zbx_strdup(host_name_temp, host_name_sample);
		goto clean;
	}

	for (i = 0; i < nums.values_num; i++)
	{
		if (num > nums.values[i])
			continue;

		if (num < nums.values[i])	/* found, all others will be bigger */
			break;

		num++;
	}

	host_name_temp = zbx_dsprintf(host_name_temp, "%s_" ZBX_FS_UI64, host_name_sample, num);
clean:
	zbx_vector_uint64_destroy(&nums);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():'%s'", __func__, host_name_temp);

	return host_name_temp;
}

/******************************************************************************
 *                                                                            *
 * Purpose: construct insert statement                                        *
 *                                                                            *
 * Return value: "<id>" if id not equal zero,                                 *
 *               otherwise "null"                                             *
 *                                                                            *
 ******************************************************************************/
const char	*zbx_db_sql_id_ins(zbx_uint64_t id)
{
	static unsigned char	n = 0;
	static char		buf[4][21];	/* 20 - value size, 1 - '\0' */
	static const char	null[5] = "null";

	if (0 == id)
		return null;

	n = (n + 1) & 3;

	zbx_snprintf(buf[n], sizeof(buf[n]), ZBX_FS_UI64, id);

	return buf[n];
}

/******************************************************************************
 *                                                                            *
 * Purpose: get corresponding host_inventory field name                       *
 *                                                                            *
 * Parameters: inventory_link - [IN] field link 1..HOST_INVENTORY_FIELD_COUNT *
 *                                                                            *
 * Return value: field name or NULL if value of inventory_link is incorrect   *
 *                                                                            *
 ******************************************************************************/
const char	*zbx_db_get_inventory_field(unsigned char inventory_link)
{
	static const char	*inventory_fields[HOST_INVENTORY_FIELD_COUNT] =
	{
		"type", "type_full", "name", "alias", "os", "os_full", "os_short", "serialno_a", "serialno_b", "tag",
		"asset_tag", "macaddress_a", "macaddress_b", "hardware", "hardware_full", "software", "software_full",
		"software_app_a", "software_app_b", "software_app_c", "software_app_d", "software_app_e", "contact",
		"location", "location_lat", "location_lon", "notes", "chassis", "model", "hw_arch", "vendor",
		"contract_number", "installer_name", "deployment_status", "url_a", "url_b", "url_c", "host_networks",
		"host_netmask", "host_router", "oob_ip", "oob_netmask", "oob_router", "date_hw_purchase",
		"date_hw_install", "date_hw_expiry", "date_hw_decomm", "site_address_a", "site_address_b",
		"site_address_c", "site_city", "site_state", "site_country", "site_zip", "site_rack", "site_notes",
		"poc_1_name", "poc_1_email", "poc_1_phone_a", "poc_1_phone_b", "poc_1_cell", "poc_1_screen",
		"poc_1_notes", "poc_2_name", "poc_2_email", "poc_2_phone_a", "poc_2_phone_b", "poc_2_cell",
		"poc_2_screen", "poc_2_notes"
	};

	if (1 > inventory_link || inventory_link > HOST_INVENTORY_FIELD_COUNT)
		return NULL;

	return inventory_fields[inventory_link - 1];
}

int	zbx_db_table_exists(const char *table_name)
{
	char		*table_name_esc;
	zbx_db_result_t	result;
	int		ret;

	table_name_esc = zbx_db_dyn_escape_string(table_name);

#if defined(HAVE_MYSQL)
	result = zbx_db_select("show tables like '%s'", table_name_esc);
#elif defined(HAVE_POSTGRESQL)
	result = zbx_db_select(
			"select 1"
			" from information_schema.tables"
			" where table_name='%s'"
				" and table_schema='%s'",
			table_name_esc, zbx_db_get_schema_esc());
#elif defined(HAVE_SQLITE3)
	result = zbx_db_select(
			"select 1"
			" from sqlite_master"
			" where tbl_name='%s'"
				" and type='table'",
			table_name_esc);
#endif

	zbx_free(table_name_esc);

	ret = (NULL == zbx_db_fetch(result) ? FAIL : SUCCEED);

	zbx_db_free_result(result);

	return ret;
}

int	zbx_db_field_exists(const char *table_name, const char *field_name)
{
#if (defined(HAVE_MYSQL) || defined(HAVE_POSTGRESQL) || defined(HAVE_SQLITE3))
	zbx_db_result_t	result;
#endif
#if defined(HAVE_MYSQL)
	char		*field_name_esc;
	int		ret;
#elif defined(HAVE_POSTGRESQL)
	char		*table_name_esc, *field_name_esc;
	int		ret;
#elif defined(HAVE_SQLITE3)
	char		*table_name_esc;
	zbx_db_row_t	row;
	int		ret = FAIL;
#endif

#if defined(HAVE_MYSQL)
	field_name_esc = zbx_db_dyn_escape_string(field_name);

	result = zbx_db_select("show columns from %s like '%s'",
			table_name, field_name_esc);

	zbx_free(field_name_esc);

	ret = (NULL == zbx_db_fetch(result) ? FAIL : SUCCEED);

	zbx_db_free_result(result);
#elif defined(HAVE_POSTGRESQL)
	table_name_esc = zbx_db_dyn_escape_string(table_name);
	field_name_esc = zbx_db_dyn_escape_string(field_name);

	result = zbx_db_select(
			"select 1"
			" from information_schema.columns"
			" where table_name='%s'"
				" and column_name='%s'"
				" and table_schema='%s'",
			table_name_esc, field_name_esc, zbx_db_get_schema_esc());

	zbx_free(field_name_esc);
	zbx_free(table_name_esc);

	ret = (NULL == zbx_db_fetch(result) ? FAIL : SUCCEED);

	zbx_db_free_result(result);
#elif defined(HAVE_SQLITE3)
	table_name_esc = zbx_db_dyn_escape_string(table_name);

	result = zbx_db_select("PRAGMA table_info('%s')", table_name_esc);

	zbx_free(table_name_esc);

	while (NULL != (row = zbx_db_fetch(result)))
	{
		if (0 != strcmp(field_name, row[1]))
			continue;

		ret = SUCCEED;
		break;
	}
	zbx_db_free_result(result);
#endif

	return ret;
}

#ifndef HAVE_SQLITE3
int	zbx_db_trigger_exists(const char *table_name, const char *trigger_name)
{
	char		*table_name_esc, *trigger_name_esc;
	zbx_db_result_t	result;
	int		ret;

	table_name_esc = zbx_db_dyn_escape_string(table_name);
	trigger_name_esc = zbx_db_dyn_escape_string(trigger_name);

#if defined(HAVE_MYSQL)
	result = zbx_db_select(
			"show triggers where `table`='%s'"
			" and `trigger`='%s'",
			table_name_esc, trigger_name_esc);
#elif defined(HAVE_POSTGRESQL)
	result = zbx_db_select(
			"select 1"
			" from information_schema.triggers"
			" where event_object_table='%s'"
			" and trigger_name='%s'"
			" and trigger_schema='%s'",
			table_name_esc, trigger_name_esc, zbx_db_get_schema_esc());
#endif
	ret = (NULL == zbx_db_fetch(result) ? FAIL : SUCCEED);

	zbx_db_free_result(result);

	zbx_free(table_name_esc);
	zbx_free(trigger_name_esc);

	return ret;
}

int	zbx_db_index_exists(const char *table_name, const char *index_name)
{
	char		*table_name_esc, *index_name_esc;
	zbx_db_result_t	result;
	int		ret;

	table_name_esc = zbx_db_dyn_escape_string(table_name);
	index_name_esc = zbx_db_dyn_escape_string(index_name);

#if defined(HAVE_MYSQL)
	result = zbx_db_select(
			"show index from %s"
			" where key_name='%s'",
			table_name_esc, index_name_esc);
#elif defined(HAVE_POSTGRESQL)
	result = zbx_db_select(
			"select 1"
			" from pg_indexes"
			" where tablename='%s'"
				" and indexname='%s'"
				" and schemaname='%s'",
			table_name_esc, index_name_esc, zbx_db_get_schema_esc());
#endif

	ret = (NULL == zbx_db_fetch(result) ? FAIL : SUCCEED);

	zbx_db_free_result(result);

	zbx_free(table_name_esc);
	zbx_free(index_name_esc);

	return ret;
}

int	zbx_db_pk_exists(const char *table_name)
{
	zbx_db_result_t	result;
	int		ret;

#if defined(HAVE_MYSQL)
	result = zbx_db_select(
			"show index from %s"
			" where key_name='PRIMARY'",
			table_name);
#elif defined(HAVE_POSTGRESQL)
	result = zbx_db_select(
			"select 1"
			" from information_schema.table_constraints"
			" where table_name='%s'"
				" and constraint_type='PRIMARY KEY'"
				" and constraint_schema='%s'",
			table_name, zbx_db_get_schema_esc());
#endif
	ret = (NULL == zbx_db_fetch(result) ? FAIL : SUCCEED);

	zbx_db_free_result(result);

	return ret;
}

#endif

/******************************************************************************
 *                                                                            *
 * Parameters: sql - [IN] sql statement                                       *
 *             ids - [OUT] sorted list of selected uint64 values              *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_select_uint64(const char *sql, zbx_vector_uint64_t *ids)
{
	zbx_db_result_t	result;
	zbx_db_row_t	row;
	zbx_uint64_t	id;

	result = zbx_db_select("%s", sql);

	while (NULL != (row = zbx_db_fetch(result)))
	{
		ZBX_STR2UINT64(id, row[0]);

		zbx_vector_uint64_append(ids, id);
	}
	zbx_db_free_result(result);

	zbx_vector_uint64_sort(ids, ZBX_DEFAULT_UINT64_COMPARE_FUNC);
}

int	zbx_db_prepare_multiple_query(const char *query, const char *field_name, zbx_vector_uint64_t *ids, char **sql,
		size_t	*sql_alloc, size_t *sql_offset)
{
#define ZBX_MAX_IDS	950
	int	i, ret = SUCCEED;

	for (i = 0; i < ids->values_num; i += ZBX_MAX_IDS)
	{
		zbx_strcpy_alloc(sql, sql_alloc, sql_offset, query);
		zbx_db_add_condition_alloc(sql, sql_alloc, sql_offset, field_name, &ids->values[i],
				MIN(ZBX_MAX_IDS, ids->values_num - i));
		zbx_strcpy_alloc(sql, sql_alloc, sql_offset, ";\n");

		if (SUCCEED != (ret = zbx_db_execute_overflowed_sql(sql, sql_alloc, sql_offset)))
			break;
	}

	return ret;
}

int	zbx_db_execute_multiple_query(const char *query, const char *field_name, zbx_vector_uint64_t *ids)
{
	char	*sql = NULL;
	size_t	sql_alloc = ZBX_KIBIBYTE, sql_offset = 0;
	int	ret = SUCCEED;

	sql = (char *)zbx_malloc(sql, sql_alloc);

	ret = zbx_db_prepare_multiple_query(query, field_name, ids, &sql, &sql_alloc, &sql_offset);

	if (SUCCEED == ret && ZBX_DB_OK > zbx_db_flush_overflowed_sql(sql, sql_offset))
		ret = FAIL;

	zbx_free(sql);

	return ret;
}

#if defined(HAVE_MYSQL) || defined(HAVE_POSTGRESQL)
static void	zbx_warn_char_set(const char *db_name, const char *char_set)
{
	zabbix_log(LOG_LEVEL_WARNING, "Zabbix supports only \"" ZBX_SUPPORTED_DB_CHARACTER_SET "\" character set(s)."
			" Database \"%s\" has default character set \"%s\"", db_name, char_set);
}
#endif

#if defined(HAVE_MYSQL) || defined(HAVE_POSTGRESQL)
static void	zbx_warn_no_charset_info(const char *db_name)
{
	zabbix_log(LOG_LEVEL_WARNING, "Cannot get database \"%s\" character set", db_name);
}
#endif

#if defined(HAVE_MYSQL)
static char	*db_strlist_quote(const char *strlist, char delimiter)
{
	const char	*delim;
	char		*str = NULL;
	size_t		str_alloc = 0, str_offset = 0;

	while (NULL != (delim = strchr(strlist, delimiter)))
	{
		zbx_snprintf_alloc(&str, &str_alloc, &str_offset, "'%.*s',", (int)(delim - strlist), strlist);
		strlist = delim + 1;
	}

	zbx_snprintf_alloc(&str, &str_alloc, &str_offset, "'%s'", strlist);

	return str;
}
#endif

void	zbx_db_check_character_set(void)
{
#if defined(HAVE_MYSQL)
	char		*database_name_esc, *charset_list, *collation_list;
	zbx_db_result_t	result;
	zbx_db_row_t	row;

	database_name_esc = zbx_db_dyn_escape_string(zbx_cfg_dbhigh->config_dbname);

	result = zbx_db_select(
			"select default_character_set_name,default_collation_name"
			" from information_schema.SCHEMATA"
			" where schema_name='%s'", database_name_esc);

	if (NULL == result || NULL == (row = zbx_db_fetch(result)))
	{
		zbx_warn_no_charset_info(zbx_cfg_dbhigh->config_dbname);
	}
	else
	{
		char	*char_set = row[0];
		char	*collation = row[1];

		if (FAIL == zbx_str_in_list(ZBX_SUPPORTED_DB_CHARACTER_SET, char_set, ZBX_DB_STRLIST_DELIM))
			zbx_warn_char_set(zbx_cfg_dbhigh->config_dbname, char_set);

		if (SUCCEED != zbx_str_in_list(ZBX_SUPPORTED_DB_COLLATION, collation, ZBX_DB_STRLIST_DELIM))
		{
			zabbix_log(LOG_LEVEL_WARNING, "Zabbix supports only \"%s\" collation(s)."
					" Database \"%s\" has default collation \"%s\"", ZBX_SUPPORTED_DB_COLLATION,
					zbx_cfg_dbhigh->config_dbname, collation);
		}
	}

	zbx_db_free_result(result);

	charset_list = db_strlist_quote(ZBX_SUPPORTED_DB_CHARACTER_SET, ZBX_DB_STRLIST_DELIM);
	collation_list = db_strlist_quote(ZBX_SUPPORTED_DB_COLLATION, ZBX_DB_STRLIST_DELIM);

	result = zbx_db_select(
			"select count(*)"
			" from information_schema.`COLUMNS`"
			" where table_schema='%s'"
				" and data_type in ('text','varchar','longtext')"
				" and (character_set_name not in (%s) or collation_name not in (%s))",
			database_name_esc, charset_list, collation_list);

	zbx_free(collation_list);
	zbx_free(charset_list);

	if (NULL == result || NULL == (row = zbx_db_fetch(result)))
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot get character set of database \"%s\" tables",
				zbx_cfg_dbhigh->config_dbname);
	}
	else if (0 != strcmp("0", row[0]))
	{
		zabbix_log(LOG_LEVEL_WARNING, "character set name or collation name that is not supported by Zabbix"
				" found in %s column(s) of database \"%s\"", row[0], zbx_cfg_dbhigh->config_dbname);
		zabbix_log(LOG_LEVEL_WARNING, "only character set(s) \"%s\" and corresponding collation(s) \"%s\""
				" should be used in database", ZBX_SUPPORTED_DB_CHARACTER_SET,
				ZBX_SUPPORTED_DB_COLLATION);
	}

	zbx_db_free_result(result);
	zbx_free(database_name_esc);
#elif defined(HAVE_POSTGRESQL)
#define OID_LENGTH_MAX		20

	char		*database_name_esc, oid[OID_LENGTH_MAX];
	zbx_db_result_t	result;
	zbx_db_row_t	row;

	database_name_esc = zbx_db_dyn_escape_string(zbx_cfg_dbhigh->config_dbname);

	result = zbx_db_select(
			"select pg_encoding_to_char(encoding)"
			" from pg_database"
			" where datname='%s'",
			database_name_esc);

	if (NULL == result || NULL == (row = zbx_db_fetch(result)))
	{
		zbx_warn_no_charset_info(zbx_cfg_dbhigh->config_dbname);
		goto out;
	}
	else if (strcasecmp(row[0], ZBX_SUPPORTED_DB_CHARACTER_SET))
	{
		zbx_warn_char_set(zbx_cfg_dbhigh->config_dbname, row[0]);
		goto out;

	}

	zbx_db_free_result(result);

	result = zbx_db_select(
			"select oid"
			" from pg_namespace"
			" where nspname='%s'",
			zbx_db_get_schema_esc());

	if (NULL == result || NULL == (row = zbx_db_fetch(result)) || '\0' == **row)
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot get character set of database \"%s\" fields",
				zbx_cfg_dbhigh->config_dbname);
		goto out;
	}

	zbx_strscpy(oid, *row);

	zbx_db_free_result(result);

	result = zbx_db_select(
			"select count(*)"
			" from pg_attribute as a"
				" left join pg_class as c"
					" on c.relfilenode=a.attrelid"
				" left join pg_collation as l"
					" on l.oid=a.attcollation"
			" where atttypid in (25,1043)"
				" and c.relnamespace=%s"
				" and c.relam=0"
				" and l.collname<>'default'",
			oid);

	if (NULL == result || NULL == (row = zbx_db_fetch(result)))
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot get character set of database \"%s\" fields",
				zbx_cfg_dbhigh->config_dbname);
	}
	else if (0 != strcmp("0", row[0]))
	{
		zabbix_log(LOG_LEVEL_WARNING, "database has %s fields with unsupported character set. Zabbix supports"
				" only \"%s\" character set", row[0], ZBX_SUPPORTED_DB_CHARACTER_SET);
	}

	zbx_db_free_result(result);

	result = zbx_db_select("show client_encoding");

	if (NULL == result || NULL == (row = zbx_db_fetch(result)))
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot get info about database \"%s\" client encoding",
				zbx_cfg_dbhigh->config_dbname);
	}
	else if (0 != strcasecmp(row[0], ZBX_SUPPORTED_DB_CHARACTER_SET))
	{
		zabbix_log(LOG_LEVEL_WARNING, "client_encoding for database \"%s\" is \"%s\". Zabbix supports only"
				" \"%s\"", zbx_cfg_dbhigh->config_dbname, row[0], ZBX_SUPPORTED_DB_CHARACTER_SET);
	}

	zbx_db_free_result(result);

	result = zbx_db_select("show server_encoding");

	if (NULL == result || NULL == (row = zbx_db_fetch(result)))
	{
		zabbix_log(LOG_LEVEL_WARNING, "cannot get info about database \"%s\" server encoding",
				zbx_cfg_dbhigh->config_dbname);
	}
	else if (0 != strcasecmp(row[0], ZBX_SUPPORTED_DB_CHARACTER_SET))
	{
		zabbix_log(LOG_LEVEL_WARNING, "server_encoding for database \"%s\" is \"%s\". Zabbix supports only"
				" \"%s\"", zbx_cfg_dbhigh->config_dbname, row[0], ZBX_SUPPORTED_DB_CHARACTER_SET);
	}
out:
	zbx_db_free_result(result);
	zbx_free(database_name_esc);
#endif
}

/******************************************************************************
 *                                                                            *
 * Purpose: releases resources allocated by bulk insert operations            *
 *                                                                            *
 * Parameters: self        - [IN] the bulk insert data                        *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_insert_clean(zbx_db_insert_t *self)
{
	for (int i = 0; i < self->rows.values_num; i++)
	{
		zbx_db_value_t	*row = self->rows.values[i];

		for (int j = 0; j < self->fields.values_num; j++)
		{
			zbx_db_field_t	*field = self->fields.values[j];

			switch (field->type)
			{
				case ZBX_TYPE_CHAR:
				case ZBX_TYPE_TEXT:
				case ZBX_TYPE_LONGTEXT:
				case ZBX_TYPE_CUID:
				case ZBX_TYPE_BLOB:
					zbx_free(row[j].str);
					break;
			}
		}

		zbx_free(row);
	}

	zbx_vector_db_value_ptr_destroy(&self->rows);

	zbx_vector_db_field_ptr_destroy(&self->fields);
}

/******************************************************************************
 *                                                                            *
 * Purpose: prepare for database bulk insert operation                        *
 *                                                                            *
 * Parameters: self        - [IN] the bulk insert data                        *
 *             table       - [IN] the target table name                       *
 *             fields      - [IN] names of the fields to insert               *
 *             fields_num  - [IN] the number of items in fields array         *
 *                                                                            *
 * Comments: The operation fails if the target table does not have the        *
 *           specified fields defined in its schema.                          *
 *                                                                            *
 *           Usage example:                                                   *
 *             zbx_db_insert_t ins;                                           *
 *                                                                            *
 *             zbx_db_insert_prepare(&ins, "history", "id", "value");         *
 *             zbx_db_insert_add_values(&ins, (zbx_uint64_t)1, 1.0);          *
 *             zbx_db_insert_add_values(&ins, (zbx_uint64_t)2, 2.0);          *
 *               ...                                                          *
 *             zbx_db_insert_execute(&ins);                                   *
 *             zbx_db_insert_clean(&ins);                                     *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_insert_prepare_dyn(zbx_db_insert_t *self, const zbx_db_table_t *table, const zbx_db_field_t **fields,
		int fields_num)
{
	if (0 == fields_num)
	{
		THIS_SHOULD_NEVER_HAPPEN;
		exit(EXIT_FAILURE);
	}

	self->autoincrement = -1;
	self->lastid = 0;

	zbx_vector_db_field_ptr_create(&self->fields);
	zbx_vector_db_value_ptr_create(&self->rows);

	self->table = table;

	for (int i = 0; i < fields_num; i++)
		zbx_vector_db_field_ptr_append(&self->fields, (zbx_db_field_t *)fields[i]);
}

/******************************************************************************
 *                                                                            *
 * Purpose: prepare for database bulk insert operation                        *
 *                                                                            *
 * Parameters: self  - [IN] the bulk insert data                              *
 *             table - [IN] the target table name                             *
 *             ...   - [IN] names of the fields to insert                     *
 *             NULL  - [IN] terminating NULL pointer                          *
 *                                                                            *
 * Comments: This is a convenience wrapper for zbx_db_insert_prepare_dyn()    *
 *           function.                                                        *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_insert_prepare(zbx_db_insert_t *self, const char *table, ...)
{
	zbx_vector_ptr_t	fields;
	va_list			args;
	char			*field;
	const zbx_db_table_t	*ptable;
	const zbx_db_field_t	*pfield;

	/* find the table and fields in database schema */
	if (NULL == (ptable = zbx_db_get_table(table)))
	{
		THIS_SHOULD_NEVER_HAPPEN;
		exit(EXIT_FAILURE);
	}

	va_start(args, table);

	zbx_vector_ptr_create(&fields);

	while (NULL != (field = va_arg(args, char *)))
	{
		if (NULL == (pfield = zbx_db_get_field(ptable, field)))
		{
			zabbix_log(LOG_LEVEL_ERR, "Cannot locate table \"%s\" field \"%s\" in database schema",
					table, field);
			THIS_SHOULD_NEVER_HAPPEN;
			exit(EXIT_FAILURE);
		}
		zbx_vector_ptr_append(&fields, (zbx_db_field_t *)pfield);
	}

	va_end(args);

	zbx_db_insert_prepare_dyn(self, ptable, (const zbx_db_field_t **)fields.values, fields.values_num);

	zbx_vector_ptr_destroy(&fields);
}

/******************************************************************************
 *                                                                            *
 * Purpose: adds row values for database bulk insert operation                *
 *                                                                            *
 * Parameters: self        - [IN] the bulk insert data                        *
 *             values      - [IN] the values to insert                        *
 *             fields_num  - [IN] the number of items in values array         *
 *                                                                            *
 * Comments: The values must be listed in the same order as the field names   *
 *           for insert preparation functions.                                *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_insert_add_values_dyn(zbx_db_insert_t *self, zbx_db_value_t **values, int values_num)
{
	int		i;
	zbx_db_value_t	*row;

	if (values_num != self->fields.values_num)
	{
		THIS_SHOULD_NEVER_HAPPEN;
		exit(EXIT_FAILURE);
	}

	row = (zbx_db_value_t *)zbx_malloc(NULL, self->fields.values_num * sizeof(zbx_db_value_t));

	for (i = 0; i < self->fields.values_num; i++)
	{
		zbx_db_field_t		*field = self->fields.values[i];
		const zbx_db_value_t	*value = values[i];

		switch (field->type)
		{
			case ZBX_TYPE_LONGTEXT:
			case ZBX_TYPE_CHAR:
			case ZBX_TYPE_TEXT:
			case ZBX_TYPE_CUID:
			case ZBX_TYPE_BLOB:
				row[i].str = DBdyn_escape_field_len(field, value->str, ESCAPE_SEQUENCE_ON);
				break;
			case ZBX_TYPE_INT:
			case ZBX_TYPE_FLOAT:
			case ZBX_TYPE_UINT:
			case ZBX_TYPE_ID:
			case ZBX_TYPE_SERIAL:
				row[i] = *value;
				break;
			default:
				THIS_SHOULD_NEVER_HAPPEN;
				exit(EXIT_FAILURE);
		}
	}

	zbx_vector_db_value_ptr_append(&self->rows, row);
}

/******************************************************************************
 *                                                                            *
 * Purpose: adds row values for database bulk insert operation                *
 *                                                                            *
 * Parameters: self - [IN] the bulk insert data                               *
 *             ...  - [IN] the values to insert                               *
 *                                                                            *
 * Comments: This is a convenience wrapper for zbx_db_insert_add_values_dyn() *
 *           function.                                                        *
 *           Note that the types of the passed values must conform to the     *
 *           corresponding field types.                                       *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_insert_add_values(zbx_db_insert_t *self, ...)
{
	zbx_vector_ptr_t	values;
	va_list			args;
	int			i;
	zbx_db_field_t		*field;
	zbx_db_value_t		*value;

	va_start(args, self);

	zbx_vector_ptr_create(&values);

	for (i = 0; i < self->fields.values_num; i++)
	{
		field = (zbx_db_field_t *)self->fields.values[i];

		value = (zbx_db_value_t *)zbx_malloc(NULL, sizeof(zbx_db_value_t));

		switch (field->type)
		{
			case ZBX_TYPE_CHAR:
			case ZBX_TYPE_TEXT:
			case ZBX_TYPE_LONGTEXT:
			case ZBX_TYPE_CUID:
			case ZBX_TYPE_BLOB:
				value->str = va_arg(args, char *);
				break;
			case ZBX_TYPE_INT:
				value->i32 = va_arg(args, int);
				break;
			case ZBX_TYPE_FLOAT:
				value->dbl = va_arg(args, double);
				break;
			case ZBX_TYPE_UINT:
			case ZBX_TYPE_ID:
				value->ui64 = va_arg(args, zbx_uint64_t);
				break;
			default:
				THIS_SHOULD_NEVER_HAPPEN;
				exit(EXIT_FAILURE);
		}

		zbx_vector_ptr_append(&values, value);
	}

	va_end(args);

	zbx_db_insert_add_values_dyn(self, (zbx_db_value_t **)values.values, values.values_num);

	zbx_vector_ptr_clear_ext(&values, zbx_ptr_free);
	zbx_vector_ptr_destroy(&values);
}

#if defined(HAVE_MYSQL) || defined(HAVE_POSTGRESQL)
/******************************************************************************
 *                                                                            *
 * Purpose: decodes Base64 encoded binary data and escapes it allowing it to  *
 *          be used inside sql statement                                      *
 *                                                                            *
 * Parameters: sql_insert_data     - [IN/OUT] base64 encoded unescaped data   *
 *                                                                            *
 * Comment: input data is released from memory and replaced with pointer      *
 *          to the output data.                                               *
 *                                                                            *
 ******************************************************************************/
static void	decode_and_escape_binary_value_for_sql(char **sql_insert_data)
{
	size_t	binary_data_len;
	char	*escaped_binary;

	size_t	binary_data_max_len = strlen(*sql_insert_data) * 3 / 4 + 1;
	char	*binary_data = (char*)zbx_malloc(NULL, binary_data_max_len);

	zbx_base64_decode(*sql_insert_data, binary_data, binary_data_max_len, &binary_data_len);
#if defined (HAVE_MYSQL)
	escaped_binary = (char*)zbx_malloc(NULL, 2 * binary_data_len);
	zbx_mysql_escape_bin(binary_data, escaped_binary, binary_data_len);
#elif defined (HAVE_POSTGRESQL)
	zbx_postgresql_escape_bin(binary_data, &escaped_binary, binary_data_len);
#endif
	zbx_free(binary_data);
	zbx_free(*sql_insert_data);
	*sql_insert_data = escaped_binary;
}
#endif

/******************************************************************************
 *                                                                            *
 * Purpose: executes the prepared database bulk insert operation              *
 *                                                                            *
 * Parameters: self - [IN] the bulk insert data                               *
 *                                                                            *
 * Return value: SUCCEED if the operation completed successfully or           *
 *               FAIL otherwise.                                              *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_insert_execute(zbx_db_insert_t *self)
{
	int			ret = FAIL, i, j;
	const zbx_db_field_t	*field;
	char			*sql_command, delim[2] = {',', '('}, *sql;
	size_t			sql_command_alloc = 512, sql_command_offset = 0,
				sql_alloc = 16 * ZBX_KIBIBYTE, sql_offset = 0;
#ifdef HAVE_MYSQL
	char		*sql_values = NULL;
	size_t		sql_values_alloc = 0, sql_values_offset = 0;
#endif

	if (0 == self->rows.values_num)
		return SUCCEED;

	/* process the auto increment field */
	if (-1 != self->autoincrement)
	{
		zbx_uint64_t	id;

		id = zbx_db_get_maxid_num(self->table->table, self->rows.values_num);

		for (i = 0; i < self->rows.values_num; i++)
		{
			zbx_db_value_t	*values = (zbx_db_value_t *)self->rows.values[i];

			values[self->autoincrement].ui64 = id++;
		}

		self->lastid = id - 1;
		/* reset autoincrement so execute could be retried with the same ids */
		self->autoincrement = -1;
	}

	sql = (char *)zbx_malloc(NULL, sql_alloc);
	sql_command = (char *)zbx_malloc(NULL, sql_command_alloc);

	/* create sql insert statement command */

	zbx_strcpy_alloc(&sql_command, &sql_command_alloc, &sql_command_offset, "insert into ");
	zbx_strcpy_alloc(&sql_command, &sql_command_alloc, &sql_command_offset, self->table->table);
	zbx_chrcpy_alloc(&sql_command, &sql_command_alloc, &sql_command_offset, ' ');

	for (i = 0; i < self->fields.values_num; i++)
	{
		field = (zbx_db_field_t *)self->fields.values[i];

		zbx_chrcpy_alloc(&sql_command, &sql_command_alloc, &sql_command_offset, delim[0 == i]);
		zbx_strcpy_alloc(&sql_command, &sql_command_alloc, &sql_command_offset, field->name);
	}
#ifdef HAVE_MYSQL
	/* MySQL workaround - explicitly add missing text fields with '' default value */
	for (field = self->table->fields; NULL != field->name; field++)
	{
		switch (field->type)
		{
			case ZBX_TYPE_BLOB:
			case ZBX_TYPE_TEXT:
			case ZBX_TYPE_LONGTEXT:
			case ZBX_TYPE_CUID:
				if (FAIL != zbx_vector_db_field_ptr_search(&self->fields, (void *)field,
						ZBX_DEFAULT_PTR_COMPARE_FUNC))
				{
					continue;
				}

				zbx_chrcpy_alloc(&sql_command, &sql_command_alloc, &sql_command_offset, ',');
				zbx_strcpy_alloc(&sql_command, &sql_command_alloc, &sql_command_offset, field->name);
				zbx_strcpy_alloc(&sql_values, &sql_values_alloc, &sql_values_offset, ",''");
				break;
		}
	}
#endif
	zbx_strcpy_alloc(&sql_command, &sql_command_alloc, &sql_command_offset, ") values ");

	for (i = 0; i < self->rows.values_num; i++)
	{
		zbx_db_value_t	*values = (zbx_db_value_t *)self->rows.values[i];

#ifdef HAVE_MULTIROW_INSERT
		if (16 > sql_offset)
			zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, sql_command);
#else
		zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, sql_command);
#endif
		for (j = 0; j < self->fields.values_num; j++)
		{
			zbx_db_value_t	*value = &values[j];

			field = (const zbx_db_field_t *)self->fields.values[j];

			zbx_chrcpy_alloc(&sql, &sql_alloc, &sql_offset, delim[0 == j]);

			switch (field->type)
			{
				case ZBX_TYPE_CHAR:
				case ZBX_TYPE_TEXT:
				case ZBX_TYPE_LONGTEXT:
				case ZBX_TYPE_CUID:
					if (0 != (field->flags & ZBX_UPPER))
					{
						zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, "upper(\'");
					}
					else
						zbx_chrcpy_alloc(&sql, &sql_alloc, &sql_offset, '\'');

					zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, value->str);

					if (0 != (field->flags & ZBX_UPPER))
					{
						zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, "\')");
					}
					else
						zbx_chrcpy_alloc(&sql, &sql_alloc, &sql_offset, '\'');
					break;
				case ZBX_TYPE_BLOB:
					zbx_chrcpy_alloc(&sql, &sql_alloc, &sql_offset, '\'');
#if defined(HAVE_MYSQL) || defined(HAVE_POSTGRESQL)
					decode_and_escape_binary_value_for_sql(&(value->str));
#endif
					zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, value->str);
					zbx_chrcpy_alloc(&sql, &sql_alloc, &sql_offset, '\'');
					break;
				case ZBX_TYPE_INT:
					zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset, "%d", value->i32);
					break;
				case ZBX_TYPE_FLOAT:
					zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset, ZBX_FS_DBL64_SQL, value->dbl);
					break;
				case ZBX_TYPE_UINT:
					zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset, ZBX_FS_UI64,
							value->ui64);
					break;
				case ZBX_TYPE_ID:
					zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset,
							zbx_db_sql_id_ins(value->ui64));
					break;
				default:
					THIS_SHOULD_NEVER_HAPPEN;
					exit(EXIT_FAILURE);
			}
		}
#ifdef HAVE_MYSQL
		if (NULL != sql_values)
			zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, sql_values);
#endif

		zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, ")" ZBX_ROW_DL);

		if (SUCCEED != (ret = zbx_db_execute_overflowed_sql(&sql, &sql_alloc, &sql_offset)))
			goto out;
	}

	if (0 != sql_offset)
	{
#ifdef HAVE_MULTIROW_INSERT
		if (',' == sql[sql_offset - 1])
		{
			sql_offset--;
			zbx_strcpy_alloc(&sql, &sql_alloc, &sql_offset, ";\n");
		}
#endif

		if (ZBX_DB_OK > zbx_db_execute("%s", sql))
			ret = FAIL;
	}
out:
	zbx_free(sql_command);
	zbx_free(sql);
#ifdef HAVE_MYSQL
	zbx_free(sql_values);
#endif

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: executes the prepared database bulk insert operation              *
 *                                                                            *
 * Parameters: self - [IN] the bulk insert data                               *
 *                                                                            *
 ******************************************************************************/
void	zbx_db_insert_autoincrement(zbx_db_insert_t *self, const char *field_name)
{
	int	i;

	for (i = 0; i < self->fields.values_num; i++)
	{
		zbx_db_field_t	*field = (zbx_db_field_t *)self->fields.values[i];

		if (ZBX_TYPE_ID == field->type && 0 == strcmp(field_name, field->name))
		{
			self->autoincrement = i;
			return;
		}
	}

	THIS_SHOULD_NEVER_HAPPEN;
	exit(EXIT_FAILURE);
}

/******************************************************************************
 *                                                                            *
 * Purpose: return the last id assigned by autoincrement                      *
 *                                                                            *
 ******************************************************************************/
zbx_uint64_t	zbx_db_insert_get_lastid(zbx_db_insert_t *self)
{
	return self->lastid;
}

/******************************************************************************
 *                                                                            *
 * Purpose: determine is it a server or a proxy database                      *
 *                                                                            *
 * Return value: ZBX_DB_SERVER - server database                              *
 *               ZBX_DB_PROXY - proxy database                                *
 *               ZBX_DB_UNKNOWN - an error occurred                           *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_get_database_type(void)
{
	const char	*result_string;
	zbx_db_result_t	result;
	int		ret = ZBX_DB_UNKNOWN;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __func__);

	if (NULL == (result = zbx_db_select_n("select userid from users", 1)))
	{
		zabbix_log(LOG_LEVEL_DEBUG, "cannot select records from \"users\" table");
		goto out;
	}

	if (NULL != zbx_db_fetch(result))
	{
		zabbix_log(LOG_LEVEL_DEBUG, "there is at least 1 record in \"users\" table");
		ret = ZBX_DB_SERVER;
	}
	else
	{
		zabbix_log(LOG_LEVEL_DEBUG, "no records in \"users\" table");
		ret = ZBX_DB_PROXY;
	}

	zbx_db_free_result(result);
out:
	switch (ret)
	{
		case ZBX_DB_SERVER:
			result_string = "ZBX_DB_SERVER";
			break;
		case ZBX_DB_PROXY:
			result_string = "ZBX_DB_PROXY";
			break;
		case ZBX_DB_UNKNOWN:
			result_string = "ZBX_DB_UNKNOWN";
			break;
	}

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():%s", __func__, result_string);

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: locks a record in a table by its primary key and an optional      *
 *          constraint field                                                  *
 *                                                                            *
 * Parameters: table     - [IN] the target table                              *
 *             id        - [IN] primary key value                             *
 *             add_field - [IN] additional constraint field name (optional)   *
 *             add_id    - [IN] constraint field value                        *
 *                                                                            *
 * Return value: SUCCEED - the record was successfully locked                 *
 *               FAIL    - the table does not contain the specified record    *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_lock_record(const char *table, zbx_uint64_t id, const char *add_field, zbx_uint64_t add_id)
{
	zbx_db_result_t		result;
	const zbx_db_table_t	*t;
	int			ret;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __func__);

	if (0 == zbx_db_txn_level())
		zabbix_log(LOG_LEVEL_DEBUG, "%s() called outside of transaction", __func__);

	t = zbx_db_get_table(table);

	if (NULL == add_field)
	{
		result = zbx_db_select("select null from %s where %s=" ZBX_FS_UI64 ZBX_FOR_UPDATE, table, t->recid, id);
	}
	else
	{
		result = zbx_db_select("select null from %s where %s=" ZBX_FS_UI64 " and %s=" ZBX_FS_UI64 ZBX_FOR_UPDATE,
				table, t->recid, id, add_field, add_id);
	}

	if (NULL == zbx_db_fetch(result))
		ret = FAIL;
	else
		ret = SUCCEED;

	zbx_db_free_result(result);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():%s", __func__, zbx_result_string(ret));

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: locks a records in a table by its primary key                     *
 *                                                                            *
 * Parameters: table     - [IN] the target table                              *
 *             ids       - [IN] primary key values                            *
 *                                                                            *
 * Return value: SUCCEED - one or more of the specified records were          *
 *                         successfully locked                                *
 *               FAIL    - the table does not contain any of the specified    *
 *                         records or 'table' name not found                  *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_lock_records(const char *table, const zbx_vector_uint64_t *ids)
{
	zbx_db_result_t		result;
	const zbx_db_table_t	*t;
	int			ret;
	char			*sql = NULL;
	size_t			sql_alloc = 0, sql_offset = 0;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __func__);

	if (0 == zbx_db_txn_level())
		zabbix_log(LOG_LEVEL_DEBUG, "%s() called outside of transaction", __func__);

	if (NULL == (t = zbx_db_get_table(table)))
	{
		zabbix_log(LOG_LEVEL_CRIT, "%s(): cannot find table '%s'", __func__, table);
		THIS_SHOULD_NEVER_HAPPEN;
		ret = FAIL;
		goto out;
	}

	zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset, "select null from %s where", table);
	zbx_db_add_condition_alloc(&sql, &sql_alloc, &sql_offset, t->recid, ids->values, ids->values_num);

	result = zbx_db_select("%s" ZBX_FOR_UPDATE, sql);

	zbx_free(sql);

	if (NULL == zbx_db_fetch(result))
		ret = FAIL;
	else
		ret = SUCCEED;

	zbx_db_free_result(result);
out:
	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():%s", __func__, zbx_result_string(ret));

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: locks a records in a table by field name                          *
 *                                                                            *
 * Parameters: table      - [IN] the target table                             *
 *             field_name - [IN] field name                                   *
 *             ids        - [IN/OUT] IN - sorted array of IDs to lock         *
 *                                   OUT - resulting array of locked IDs      *
 *                                                                            *
 * Return value: SUCCEED - one or more of the specified records were          *
 *                         successfully locked                                *
 *               FAIL    - no records were locked                             *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_lock_ids(const char *table_name, const char *field_name, zbx_vector_uint64_t *ids)
{
	char		*sql = NULL;
	size_t		sql_alloc = 0, sql_offset = 0;
	zbx_uint64_t	id;
	int		i;
	zbx_db_result_t	result;
	zbx_db_row_t	row;

	if (0 == ids->values_num)
		return FAIL;

	zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset, "select %s from %s where", field_name, table_name);
	zbx_db_add_condition_alloc(&sql, &sql_alloc, &sql_offset, field_name, ids->values, ids->values_num);
	zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset, " order by %s" ZBX_FOR_UPDATE, field_name);
	result = zbx_db_select("%s", sql);
	zbx_free(sql);

	for (i = 0; NULL != (row = zbx_db_fetch(result)); i++)
	{
		ZBX_STR2UINT64(id, row[0]);

		while (id != ids->values[i])
			zbx_vector_uint64_remove(ids, i);
	}
	zbx_db_free_result(result);

	while (i != ids->values_num)
		zbx_vector_uint64_remove_noorder(ids, i);

	return (0 != ids->values_num ? SUCCEED : FAIL);
}

/******************************************************************************
 *                                                                            *
 * Purpose: validate that session is active and get associated user data      *
 *                                                                            *
 * Parameters: sessionid - [IN] the session id to validate                    *
 *             user      - [OUT] user information                             *
 *                                                                            *
 * Return value:  SUCCEED - session is active and user data was retrieved     *
 *                FAIL    - otherwise                                         *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_get_user_by_active_session(const char *sessionid, zbx_user_t *user)
{
	char		*sessionid_esc;
	int		ret = FAIL;
	zbx_db_result_t	result;
	zbx_db_row_t	row;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s() sessionid:%s", __func__, sessionid);

	sessionid_esc = zbx_db_dyn_escape_string(sessionid);

	if (NULL == (result = zbx_db_select(
			"select u.userid,u.roleid,u.username,r.type"
				" from sessions s,users u,role r"
			" where s.userid=u.userid"
				" and s.sessionid='%s'"
				" and s.status=%d"
				" and u.roleid=r.roleid",
			sessionid_esc, ZBX_SESSION_ACTIVE)))
	{
		goto out;
	}

	if (NULL == (row = zbx_db_fetch(result)))
		goto out;

	ZBX_STR2UINT64(user->userid, row[0]);
	ZBX_STR2UINT64(user->roleid, row[1]);
	user->username = zbx_strdup(NULL, row[2]);
	user->type = atoi(row[3]);

	ret = SUCCEED;
out:
	zbx_db_free_result(result);
	zbx_free(sessionid_esc);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():%s", __func__, zbx_result_string(ret));

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: validate that token is not expired and is active and then get     *
 *          associated user data                                              *
 *                                                                            *
 * Parameters: formatted_auth_token_hash - [IN] auth token to validate        *
 *             user                      - [OUT] user information             *
 *                                                                            *
 * Return value:  SUCCEED - token is valid and user data was retrieved        *
 *                FAIL    - otherwise                                         *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_get_user_by_auth_token(const char *formatted_auth_token_hash, zbx_user_t *user)
{
	int		ret = FAIL;
	zbx_db_result_t	result = NULL;
	zbx_db_row_t	row;
	time_t		t;

	zabbix_log(LOG_LEVEL_DEBUG, "In %s() auth token:%s", __func__, formatted_auth_token_hash);

	t = time(NULL);

	if ((time_t) - 1 == t)
	{
		zabbix_log(LOG_LEVEL_ERR, "%s(): failed to get time: %s", __func__, zbx_strerror(errno));
		goto out;
	}

	if (NULL == (result = zbx_db_select(
			"select u.userid,u.roleid,u.username,r.type"
				" from token t,users u,role r"
			" where t.userid=u.userid"
				" and t.token='%s'"
				" and u.roleid=r.roleid"
				" and t.status=%d"
				" and (t.expires_at=%d or t.expires_at > %lu)",
			formatted_auth_token_hash, ZBX_AUTH_TOKEN_ENABLED, ZBX_AUTH_TOKEN_NEVER_EXPIRES,
			(unsigned long)t)))
	{
		goto out;
	}

	if (NULL == (row = zbx_db_fetch(result)))
		goto out;

	ZBX_STR2UINT64(user->userid, row[0]);
	ZBX_STR2UINT64(user->roleid, row[1]);
	user->username = zbx_strdup(NULL, row[2]);
	user->type = atoi(row[3]);
	ret = SUCCEED;
out:
	zbx_db_free_result(result);

	zabbix_log(LOG_LEVEL_DEBUG, "End of %s():%s", __func__, zbx_result_string(ret));

	return ret;
}

void	zbx_user_init(zbx_user_t *user)
{
	user->username = NULL;
}

void	zbx_user_free(zbx_user_t *user)
{
	zbx_free(user->username);
}

/******************************************************************************
 *                                                                            *
 * Purpose: checks instanceid value in config table and generates new         *
 *          instance id if its empty                                          *
 *                                                                            *
 * Return value: SUCCEED - valid instance id either exists or was created     *
 *               FAIL    - no valid instance id exists and could not create   *
 *                         one                                                *
 *                                                                            *
 ******************************************************************************/
int	zbx_db_check_instanceid(void)
{
	zbx_db_result_t	result;
	zbx_db_row_t	row;
	int		ret = SUCCEED;

	result = zbx_db_select("select configid,instanceid from config order by configid");
	if (NULL != (row = zbx_db_fetch(result)))
	{
		if (SUCCEED == zbx_db_is_null(row[1]) || '\0' == *row[1])
		{
			char	*token;

			token = zbx_create_token(0);
			if (ZBX_DB_OK > zbx_db_execute("update config set instanceid='%s' where configid=%s", token, row[0]))
			{
				zabbix_log(LOG_LEVEL_ERR, "cannot update instance id in database");
				ret = FAIL;
			}
			zbx_free(token);
		}
	}
	else
	{
		zabbix_log(LOG_LEVEL_ERR, "cannot read instance id from database");
		ret = FAIL;
	}
	zbx_db_free_result(result);

	return ret;
}

int	zbx_db_update_software_update_checkid(void)
{
	zbx_db_result_t	result;
	zbx_db_row_t	row;
	int		ret = SUCCEED;

	result = zbx_db_select("select software_update_checkid from config");
	if (NULL != (row = zbx_db_fetch(result)))
	{
		if (SUCCEED == zbx_db_is_null(row[0]) || '\0' == *row[0])
		{
			char	*token;

			token = zbx_create_token(0);
			if (ZBX_DB_OK > zbx_db_execute("update config set software_update_checkid='%s'", token))
			{
				zabbix_log(LOG_LEVEL_ERR, "cannot update software_update_checkid in config table");
				ret = FAIL;
			}
			zbx_free(token);
		}
	}
	else
	{
		zabbix_log(LOG_LEVEL_ERR, "cannot read config record from database");
		ret = FAIL;
	}
	zbx_db_free_result(result);

	return ret;
}

#if defined(HAVE_POSTGRESQL)
/******************************************************************************
 *                                                                            *
 * Purpose: returns escaped DB schema name                                    *
 *                                                                            *
 ******************************************************************************/
char	*zbx_db_get_schema_esc(void)
{
	static char	*name;

	if (NULL == name)
	{
		name = zbx_db_dyn_escape_string(NULL == zbx_cfg_dbhigh->config_dbschema ||
				'\0' == *zbx_cfg_dbhigh->config_dbschema ? "public" : zbx_cfg_dbhigh->config_dbschema);
	}

	return name;
}
#endif

