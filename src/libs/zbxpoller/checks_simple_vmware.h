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

#ifndef ZABBIX_CHECKS_SIMPLE_VMWARE_H
#define ZABBIX_CHECKS_SIMPLE_VMWARE_H

#include "zbxpoller.h"

#if defined(HAVE_LIBXML2) && defined(HAVE_LIBCURL)
#include "zbxcacheconfig.h"

int	check_vcenter_version(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_fullname(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_eventlog(AGENT_REQUEST *request, const zbx_dc_item_t *item, AGENT_RESULT *result,
		zbx_vector_agent_result_ptr_t *add_results);

int	check_vcenter_cluster_alarms_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_cluster_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_cluster_property(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_cluster_status(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_cluster_tags_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_cl_perfcounter(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);

int	check_vcenter_alarms_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_alarms_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_tags_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_read(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_perfcounter(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_property(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_size(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_write(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_datastore_hv_list(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);

int	check_vcenter_dvswitch_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_dvswitch_fetchports_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);

int	check_vcenter_hv_alarms_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_cluster_name(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_connectionstate(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_cpu_usage(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_cpu_usage_perf(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_cpu_utilization(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_datacenter_name(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_datastore_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_datastore_read(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_datastore_size(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_datastore_write(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_datastore_list(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_datastore_multipath(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_diskinfo_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_fullname(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_cpu_num(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_cpu_freq(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_cpu_model(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_cpu_threads(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_memory(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_model(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_serialnumber(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_uuid(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_vendor(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_memory_size_ballooned(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_memory_used(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_property(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_net_if_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT * result);
int	check_vcenter_hv_network_in(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_network_out(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_network_linkspeed(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_tags_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_perfcounter(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_power(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_sensor_health_state(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_status(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_maintenance(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_uptime(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_version(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_sensors_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_hw_sensors_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_hv_vm_num(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);

int	check_vcenter_vm_alarms_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_attribute(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_cluster_name(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_cpu_num(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_consolidationneeded(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_cpu_ready(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_cpu_usage(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_cpu_latency(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_cpu_readiness(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_cpu_swapwait(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_cpu_usage_perf(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_datacenter_name(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_guest_memory_size_swapped(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_guest_uptime(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_hv_name(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size_consumed(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_usage(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size_ballooned(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size_compressed(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size_swapped(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size_usage_guest(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size_usage_host(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size_private(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_memory_size_shared(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_net_if_usage(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_property(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_powerstate(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_snapshot_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_net_if_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_net_if_in(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_net_if_out(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_perfcounter(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_state(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_storage_committed(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_storage_unshared(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_storage_uncommitted(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_tags_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_storage_readoio(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_storage_writeoio(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_storage_totalwritelatency(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_storage_totalreadlatency(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_tools(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_uptime(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_vfs_dev_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_vfs_dev_read(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_vfs_dev_write(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_vfs_fs_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_vm_vfs_fs_size(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);

int	check_vcenter_dc_alarms_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_dc_discovery(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_dc_tags_get(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);

int	check_vcenter_rp_cpu_usage(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);
int	check_vcenter_rp_memory(AGENT_REQUEST *request, const char *username, const char *password,
		AGENT_RESULT *result);

#endif	/* defined(HAVE_LIBXML2) && defined(HAVE_LIBCURL) */
#endif
