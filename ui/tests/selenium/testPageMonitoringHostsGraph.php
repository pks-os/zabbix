<?php
/*
** Zabbix
** Copyright (C) 2001-2022 Zabbix SIA
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


require_once dirname(__FILE__).'/../include/CWebTest.php';

/**
 * @backup profiles, hosts, items, graphs
 *
 * @onBefore prepareGraphsData
 */
class testPageMonitoringHostsGraph extends CWebTest {

	public function prepareGraphsData() {
		$hosts = CDataHelper::call('host.create', [
			[
				'host' => 'Host_for_monitoring_graphs_1',
				'groups' => [
					'groupid' => 4
				],
				'interfaces' => [
					'type'=> 1,
					'main' => 1,
					'useip' => 1,
					'ip' => '127.0.0.1',
					'dns' => '',
					'port' => '10050'
				]
			],
			[
				'host' => 'Host_for_monitoring_graphs_2',
				'groups' => [
					'groupid' => 4
				],
				'interfaces' => [
					'type'=> 1,
					'main' => 1,
					'useip' => 1,
					'ip' => '127.0.0.1',
					'dns' => '',
					'port' => '10050'
				]
			]
		]);
		$this->assertArrayHasKey('hostids', $hosts);
		$hostid = CDataHelper::getIds('host');

		$items = CDataHelper::call('item.create', [
			[
				'name' => 'Item_for_graph_1',
				'key_' => 'trap_1',
				'hostid' => $hostid['Host_for_monitoring_graphs_1'],
				'type' => 2,
				'value_type' => 0,
				'tags' => [
					[
						'tag' => 'tag_name_1',
						'value' => 'tag_value_1'
					]
				]
			],
			[
				'name' => 'Item_for_graph_2',
				'key_' => 'trap_2',
				'hostid' => $hostid['Host_for_monitoring_graphs_1'],
				'type' => 2,
				'value_type' => 0,
				'tags' => [
					[
						'tag' => 'tag_name_2',
						'value' => 'tag_value_2'
					]
				]
			],
			[
				'name' => 'Item_for_graph_3',
				'key_' => 'trap_3',
				'hostid' => $hostid['Host_for_monitoring_graphs_1'],
				'type' => 2,
				'value_type' => 0,
				'tags' => [
					[
						'tag' => 'tag_name_3',
						'value' => 'tag_value_3'
					]
				]
			],
			[
				'name' => 'Item_for_graph_4',
				'key_' => 'trap_4',
				'hostid' => $hostid['Host_for_monitoring_graphs_2'],
				'type' => 2,
				'value_type' => 0
			]
		]);
		$this->assertArrayHasKey('itemids', $items);
		$itemids = CDataHelper::getIds('name');

		$graphs = CDataHelper::call('graph.create', [
			[
				'name' => 'Graph_1',
				'gitems' => [
					[
						'itemid' => $itemids['Item_for_graph_1'],
						'color' => '00AA00'
					]
				]
			],
			[
				'name' => 'Graph_2',
				'gitems' => [
					[
						'itemid' => $itemids['Item_for_graph_2'],
						'color' => '00AA00'
					],
					[
						'itemid' => $itemids['Item_for_graph_2'],
						'color' => '00AA00'
					]
				]
			],
			[
				'name' => 'Graph_3',
				'gitems' => [
					[
						'itemid' => $itemids['Item_for_graph_2'],
						'color' => '00AA00'
					],
					[
						'itemid' => $itemids['Item_for_graph_3'],
						'color' => '00AA00'
					]

				]
			],
			[
				'name' => 'Graph_4',
				'gitems' => [
					[
						'itemid' => $itemids['Item_for_graph_4'],
						'color' => '00AA00'
					]
				]
			]
		]);
		$this->assertArrayHasKey('graphids', $graphs);
	}

	/**
	 * Check graph page layout.
	 */
	public function testPageMonitoringHostsGraph_Layout() {
		$this->page->login()->open('zabbix.php?view_as=showgraph&action=charts.view&from=now-1h&to'.
				'=now&filter_search_type=0&filter_set=1');
		$this->page->assertHeader('Graphs');
		$this->page->assertTitle('Custom graphs');

		// If the time selector is not visible - enable it.
		if ($this->query('xpath://li[@aria-labelledby="ui-id-1" and @aria-selected="false"]')->exists()) {
			$this->query('id:ui-id-1')->one()->click();
		}

		// Check that time selector set to display Last hour data.
		$this->assertEquals('selected', $this->query('xpath://a[@data-label="Last 1 hour"]')
			->one()->getAttribute('class')
		);

		// Enable filter and check filter labels.
		$this->query('id:ui-id-2')->one()->click();
		$form = $this->query('name:zbx_filter')->asform()->one();
		$this->assertEquals(['Hosts', 'Name', 'Show'], $form->getLabels()->asText());

		// Check Show parameters and default value.
		$radio = $this->query('id:filter_show')->asSegmentedRadio()->one();
		$this->assertEquals(['All graphs', 'Host graphs', 'Simple graphs'], $radio->getLabels()->asText());
		$this->assertEquals('All graphs', $radio->getSelected());

		// Check placeholder for Hosts.
		$this->assertEquals('type here to search', $form->query('id:filter_hostids__ms')->one()->getAttribute('placeholder'));

		// Check filter buttons.
		foreach (['Apply', 'Reset'] as $button) {
			$this->assertTrue($form->query('xpath:.//div[@class="filter-forms"]/button[text()="'.$button.'"]')
				->one()->isClickable()
			);
		}

		// When table result is empty.
		$this->assertEquals('Specify host to see the graphs.', $this->query('class:nothing-to-show')->one()->getText());
	}

	/**
	 * Check graph page in layout mode.
	 */
	public function testPageMonitoringHostsGraph_KioskMode() {
		$this->page->login()->open('zabbix.php?view_as=showgraph&action=charts.view&from=now-1h&to'.
			'=now&filter_search_type=0&filter_set=1');

		// Check Kiosk mode.
		$this->query('xpath://button[@title="Kiosk mode"]')->one()->click();
		$this->page->waitUntilReady();

		// Check that Header and Filter disappeared.
		$this->query('xpath://h1[@id="page-title-general"]')->waitUntilNotVisible();
		$this->assertFalse($this->query('xpath://div[@aria-label="Filter"]')->exists());

		// Return to normal view.
		$this->query('xpath://button[@title="Normal view"]')->waitUntilPresent()->one()->click(true);
		$this->page->waitUntilReady();

		// Check that Header and Filter are visible again.
		$this->query('xpath://h1[@id="page-title-general"]')->waitUntilVisible();
		$this->assertTrue($this->query('xpath://div[@aria-label="Filter"]')->exists());
	}

	/**
	 * Check tags filtering.
	 */
	public function testPageMonitoringHostsGraph_TagFilter() {
		//		// Check that tags and information message are visible.
//		$form->fill('Host for monitoring graphs')->submit();
//		$this->page->waitUntilReady();
	}

	public static function getCheckFilterData() {
		return [
			// #0 One host with several items and graphs. Show all graphs.
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Show' => 'All graphs'
					],
					'graphs_result' => 6
				]
			],
			// #1 Show host graphs for host.
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Show' => 'Host graphs'
					],
					'graphs_result' => 3
				]
			],
			// #2 Show simple graphs for host.
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Show' => 'Simple graphs'
					],
					'graphs_result' => 3
				]
			],
			// #3 Show all graphs for host with graph name.
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'Graph_2',
						'Show' => 'All graphs'
					],
					'graphs_result' => 3
				]
			],
			// #4 Show host graphs for host with graph name.
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'Graph_2',
						'Show' => 'Host graphs'
					],
					'graphs_result' => 3
				]
			],
			// #5 Show simple graphs for host with graph name.
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'Graph_2',
						'Show' => 'Host graphs'
					],
					'graphs_result' => 1
				]
			],
			// #6 Show all graphs with not existing graph name .
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'Nothing here',
						'Show' => 'All graphs'
					],
					'graphs_result' => 0
				]
			],
			// #7 Show host graphs with not existing graph name .
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'Nothing here',
						'Show' => 'Host graphs'
					],
					'graphs_result' => 0
				]
			],
			// #8 Show simple graphs with not existing graph name .
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'Nothing here',
						'Show' => 'Simple graphs'
					],
					'graphs_result' => 0
				]
			],
			// #9 Show all graphs with partial graph name .
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'gr',
						'Show' => 'All graphs'
					],
					'graphs_result' => 0
				]
			],
			// #10 Show host graphs with partial graph name .
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'gr',
						'Show' => 'Host graphs'
					],
					'graphs_result' => 0
				]
			],
			// #11 Show simple graphs with partial graph name .
			[
				[
					'filter' => [
						'Hosts' => 'Host_for_monitoring_graphs_1',
						'Name' => 'gr',
						'Show' => 'Simple graphs'
					],
					'graphs_result' => 0
				]
			]
		];
	}

	/**
	 * Check graph page filter.
	 *
	 * @dataProvider getCheckFilterData
	 */
	public function testPageMonitoringHostsGraph_CheckFilter($data) {
		$this->page->login()->open('zabbix.php?view_as=showgraph&action=charts.view&from=now-1h&to='.
			'now&filter_search_type=0&filter_set=1');

		// If the filter is not visible - enable it.
		if ($this->query('xpath://li[@aria-labelledby="ui-id-2" and @aria-selected="false"]')->exists()) {
			$this->query('id:ui-id-2')->one()->click();
		}

		$form = $this->query('name:zbx_filter')->one()->asForm();
		$form->fill($data['filter'])->submit();
		$this->page->waitUntilReady();

		// Check result amount.
		if ($data['graphs_result'] !== 0) {
			$graphs_count = $this->query('xpath://tbody/tr/div[@class="flickerfreescreen"]')->all()->count();
			$this->assertEquals($data['graphs_result'], $graphs_count);
		}
		else {
			$this->assertEquals('No data found.', $this->query('class:nothing-to-show')->one()->getText());
		}
	}
}
