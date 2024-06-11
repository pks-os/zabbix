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

#ifndef ZABBIX_CHECKS_SIMPLE_H
#define ZABBIX_CHECKS_SIMPLE_H

#include "zbxpoller.h"

#include "zbxcacheconfig.h"

int	get_value_simple(const zbx_dc_item_t *item, AGENT_RESULT *result, zbx_vector_agent_result_ptr_t *add_results,
		zbx_get_config_forks_f get_config_forks);

#endif
