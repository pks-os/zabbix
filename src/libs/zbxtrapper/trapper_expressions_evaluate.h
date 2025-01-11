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

#ifndef ZABBIX_TRAPPER_EXPRESSION_EVALUATE_H
#define ZABBIX_TRAPPER_EXPRESSION_EVALUATE_H

#include "zbxcomms.h"
#include "zbxjson.h"

int	zbx_trapper_expressions_evaluate(zbx_socket_t *sock, const struct zbx_json_parse *jp, int config_timeout);
int	zbx_trapper_expressions_evaluate_run(const struct zbx_json_parse *jp, struct zbx_json *json, char **error);

#endif
