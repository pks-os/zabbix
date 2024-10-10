<?php
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

require_once dirname(__FILE__).'/../common/testSystemInformation.php';

/**
 * @backup ha_node, config
 *
 * @backupConfig
 */
class testPageReportsSystemInformation extends testSystemInformation {

// Commented until Jenkins issue investigated.
//	public function testPageReportsSystemInformation_checkDisabledHA() {
//		$this->page->login()->open('zabbix.php?action=report.status')->waitUntilReady();
//		$this->assertScreenshotExcept(null,
//				[$this->query('xpath://footer')->one(),
//				$this->query('xpath://table[@class="list-table sticky-header"]/tbody/tr[3]/td[1]')->one()],
//				'report_without_ha');
//	}

	/**
	 * @onBefore prepareHANodeData
	 */
	public function testPageReportsSystemInformation_checkEnabledHA() {
		$this->assertEnabledHACluster();
		$this->assertScreenshotExcept(null, self::$skip_fields, 'report_with_ha');
	}

	/**
	 * Function checks that zabbix server status is updated after failover delay passes and frontend config is re-validated.
	 *
	 * @depends testPageReportsSystemInformation_checkEnabledHA
	 *
	 * @onBefore changeFailoverDelay
	 */
	public function testPageReportsSystemInformation_CheckServerStatus() {
		$this->assertServerStatusAfterFailover();
	}
}
