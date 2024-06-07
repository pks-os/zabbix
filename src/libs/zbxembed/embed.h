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

#ifndef ZABBIX_EMBED_H
#define ZABBIX_EMBED_H

#include "zbxembed.h"
#include "duktape.h"
#include "zbxtime.h"
#include "zbxalgo.h"

#define ZBX_ES_LOG_MEMORY_LIMIT	(ZBX_MEBIBYTE * 8)
#define ZBX_ES_LOG_MSG_LIMIT	8000

/* this macro can be used in time intensive C functions to check for script timeout execution */
#define ZBX_ES_CHECK_TIMEOUT(ctx, env)									\
	do												\
	{												\
		zbx_uint64_t	elapsed_ms;								\
		elapsed_ms = zbx_get_duration_ms(&env->start_time);					\
		if (elapsed_ms >= (zbx_uint64_t)env->timeout * 1000)					\
			return duk_error(ctx, DUK_RET_TYPE_ERROR, "script execution timeout occurred");	\
	}												\
	while (0)

struct zbx_es_env
{
	duk_context	*ctx;
	size_t		total_alloc;
	size_t		max_total_alloc;
	zbx_timespec_t	start_time;

	char		*error;
	int		rt_error_num;
	int		fatal_error;
	int		timeout;
	struct zbx_json	*json;

	jmp_buf		loc;

	int		http_req_objects;

	int		logged_msgs;

	const char	*config_source_ip;

	char		*browser_endpoint;

	int		browser_objects;
	int		constructor_chain;

	zbx_hashset_t	objmap;
};

zbx_es_env_t	*zbx_es_get_env(duk_context *ctx);

int	es_duktape_string_decode(const char *duk_str, char **out_str);

duk_ret_t	es_super(duk_context *ctx, const char *base, int args);
int	es_is_chained_constructor_call(duk_context *ctx);

void	es_obj_attach_data(zbx_es_env_t *env, void *data);
void	*es_obj_get_data(zbx_es_env_t *env);
void	*es_obj_detach_data(zbx_es_env_t *env);

#endif /* ZABBIX_EMBED_H */
