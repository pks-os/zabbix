<?php
/*
** Zabbix
** Copyright (C) 2001-2024 Zabbix SIA
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


require_once dirname(__FILE__).'/../include/CAPITest.php';

/**
 * @backup triggers
 *
 * @onBefore prepare_triggerprototype_data
 * @onAfter cleanup_triggerprototype_data
 */
class testTriggerPrototypes extends CAPITest {

	public function prepare_triggerprototype_data(): void {
		$result = $this->call('itemprototype.create', [
			'hostid' => '50009',
			'ruleid' => '400660',
			'name' => 'master.item',
			'key_' => 'master.item[{#LLD}]',
			'type' => ITEM_TYPE_ZABBIX,
			'value_type' => ITEM_VALUE_TYPE_FLOAT,
			'interfaceid' => 50022,
			'delay' => '1m'
		]);
		$master_itemid = reset($result['result']['itemids']);

		$result = $this->call('itemprototype.create', [
			'hostid' => '50009',
			'ruleid' => '400660',
			'name' => 'binary.item',
			'key_' => 'binary.item[{#LLD}]',
			'type' => ITEM_TYPE_DEPENDENT,
			'master_itemid' => $master_itemid,
			'value_type' => ITEM_VALUE_TYPE_BINARY
		]);
	}

	public function cleanup_triggerprototype_data(): void {
		$result = $this->call('item.get', [
			'filter' => [
				'key_' => 'master.item'
			]
		]);
		$master_itemid = reset($result['result']['itemids']);

		$this->call('item.delete', [$master_itemid]);
	}

	public static function trigger_prototype_create_data() {
		return [
			'Prohibit binary items in expression' => [
				'params' => [
					'description' => 'trigger.error',
					'expression' => 'last(/API Host/binary.item[{#LLD}])=0'
				],
				'error' => 'Incorrect item value type "Binary" provided for trigger function "last".'
			],
			'Prohibit binary items in recovery expression' => [
				'params' => [
					'description' => 'trigger.error',
					'recovery_mode' => ZBX_RECOVERY_MODE_RECOVERY_EXPRESSION,
					'expression' => 'last(/API Host/master.item[{#LLD}])=0',
					'recovery_expression' => 'last(/API Host/binary.item[{#LLD}])=0'
				],
				'error' => 'Incorrect item value type "Binary" provided for trigger function "last".'
			]
		];
	}

	/**
	* @dataProvider trigger_prototype_create_data
	*/
	public function testTriggerPrototype_Create($params, $expected_error) {
		$this->call('triggerprototype.create', $params, $expected_error);
	}
}
