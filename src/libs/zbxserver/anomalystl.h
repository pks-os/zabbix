/*
** Zabbix
** Copyright (C) 2001-2021 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#ifndef ZABBIX_ANOMALYSTL_H
#define ZABBIX_ANOMALYSTL_H

#include "common.h"

#include "evalfunc_common.h"

#define STL_DEF_DEVIATIONS	3
#define S_DEGREE_DEF		0
#define S_WINDOW_DEF		0
#define T_WINDOW_DEF		ZBX_INFINITY
#define T_DEGREE_DEF		1
#define L_WINDOW_DEF		-1
#define L_DEGREE_DEF		-1
#define S_JUMP_DEF		-1
#define T_JUMP_DEF		-1
#define L_JUMP_DEF		-1
#define ROBUST_DEF		0
#define INNER_DEF		-1
#define OUTER_DEF		-1

int	zbx_STL(zbx_vector_history_record_t *ts, int freq, int is_robust, int s_window, int s_degree, double t_window, int t_degree,
		int l_window, int l_degree, int s_jump, int t_jump, int l_jump, int inner, int outer,
		zbx_vector_history_record_t *trend, zbx_vector_history_record_t *seasonal,
		zbx_vector_history_record_t *remainder, char **error);

int	zbx_get_percentage_of_deviations_in_stl_remainder(const zbx_vector_history_record_t *remainder,
		zbx_uint64_t deviations_count, const char* devalg, int detect_period_start, int detect_period_end,
		double *result, char **error);
#endif
