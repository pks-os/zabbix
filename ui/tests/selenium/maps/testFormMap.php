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


require_once dirname(__FILE__).'/../../include/CLegacyWebTest.php';

/**
 * @backup sysmaps
 */
class testFormMap extends CLegacyWebTest {
	/**
	 * Possible combinations of grid settings
	 * @return array
	 */
	public function possibleGridOptions() {
		return [
			// Array value description: grid dimensions, show grid, auto align.
			['20x20', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_ON],
			['40x40', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_ON],
			['50x50', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_ON],
			['75x75', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_ON],
			['100x100', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_ON],

			['20x20', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_OFF],
			['40x40', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_OFF],
			['50x50', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_OFF],
			['75x75', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_OFF],
			['100x100', SYSMAP_GRID_SHOW_ON, SYSMAP_GRID_ALIGN_OFF],

			['20x20', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_ON],
			['40x40', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_ON],
			['50x50', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_ON],
			['75x75', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_ON],
			['100x100', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_ON],

			['20x20', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_OFF],
			['40x40', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_OFF],
			['50x50', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_OFF],
			['75x75', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_OFF],
			['100x100', SYSMAP_GRID_SHOW_OFF, SYSMAP_GRID_ALIGN_OFF]
		];
	}

	public static function allMaps() {
		return CDBHelper::getDataProvider('SELECT * FROM sysmaps');
	}

	/**
	 * @dataProvider allMaps
	 *
	 * @browsers chrome
	 */
	public function testFormMap_SimpleUpdateConstructor($map) {
		$sysmapid = $map['sysmapid'];

		$sql_maps_elements = 'SELECT * FROM sysmaps sm INNER JOIN sysmaps_elements sme ON'.
				' sme.sysmapid = sm.sysmapid ORDER BY sme.selementid';
		$sql_links_triggers = 'SELECT * FROM sysmaps_links sl INNER JOIN sysmaps_link_triggers slt ON'.
				' slt.linkid = sl.linkid ORDER BY slt.linktriggerid';
		$hash_maps_elements = CDBHelper::getHash($sql_maps_elements);
		$hash_links_triggers = CDBHelper::getHash($sql_links_triggers);

		$this->page->login()->open('sysmaps.php')->waitUntilReady();
		$this->query('link', $map['name'])->one()->click();
		$this->page->waitUntilReady();

		$element = $this->query('xpath://div[@id="flickerfreescreen_mapimg"]/div/*[name()="svg"]')
				->waitUntilPresent()->one();

		$exclude = [
			'query'	=> 'class:map-timestamp',
			'color'	=> '#ffffff'
		];
		$this->assertScreenshotExcept($element, $exclude, 'view_'.$sysmapid);

		$this->query('button:Edit map')->one()->click();
		$this->page->waitUntilReady();
		$this->assertScreenshot($this->query('id:map-area')->waitUntilPresent()->one(), 'edit_'.$sysmapid);
		$this->query('button:Update')->one()->click();

		$this->page->waitUntilAlertIsPresent();
		$this->assertEquals('Map is updated! Return to map list?', $this->page->getAlertText());
		$this->page->acceptAlert();

		$this->page->waitUntilReady();
		$this->assertStringContainsString('sysmaps.php', $this->page->getCurrentUrl());

		$hash_data = [
			$hash_maps_elements => $sql_maps_elements,
			$hash_links_triggers => $sql_links_triggers
		];

		foreach ($hash_data as $old => $new) {
			$this->assertEquals($old, CDBHelper::getHash($new));
		}
	}

	/**
	 * Test setting of grid options for map
	 *
	 * @dataProvider possibleGridOptions
	 */
	public function testFormMap_UpdateGridOptions($gridSize, $showGrid, $autoAlign) {

		$map_name = 'Test map 1';

		// getting map options from DB as they are at the beginning of the test
		$db_map = CDBHelper::getRow('SELECT * FROM sysmaps WHERE name='.zbx_dbstr($map_name));
		$this->assertTrue(isset($db_map['sysmapid']));

		$this->zbxTestLogin('sysmaps.php');
		$this->zbxTestCheckTitle('Configuration of network maps');
		$this->zbxTestClickLinkTextWait($map_name);
		$this->zbxTestContentControlButtonClickTextWait('Edit map');

		// checking if appropriate value for grid size is selected
		$this->assertTrue($this->zbxTestGetValue('//z-select[@id="gridsize"]') == $db_map['grid_size']);

		// grid should be shown by default
		if ($db_map['grid_show'] == SYSMAP_GRID_SHOW_ON) {
			$this->zbxTestTextPresent('Shown');
			$this->zbxTestTextNotPresent('Hidden');
		}
		else {
			$this->zbxTestTextPresent('Hidden');
			$this->zbxTestTextNotPresent('Shown');
		}

		// auto align should be on by default
		if ($db_map['grid_align'] == SYSMAP_GRID_ALIGN_ON) {
			$this->zbxTestAssertElementText("//button[@id='gridautoalign']", 'On');
		}
		else {
			$this->zbxTestAssertElementText("//button[@id='gridautoalign']", 'Off');
		}

		// selecting new grid size
		$this->zbxTestDropdownSelect('gridsize', $gridSize);
		$this->zbxTestAssertElementValue('gridsize', strstr($gridSize, 'x', true));

		// changing other two options if they are not already set as needed
		if (($db_map['grid_show'] == SYSMAP_GRID_SHOW_ON && $showGrid == 0) || ($db_map['grid_show'] == SYSMAP_GRID_SHOW_OFF && $showGrid == 1)) {
			$this->zbxTestClickWait('gridshow');
		}
		if (($db_map['grid_align'] == SYSMAP_GRID_ALIGN_ON && $autoAlign == 0) || ($db_map['grid_align'] == SYSMAP_GRID_ALIGN_OFF && $autoAlign == 1)) {
			$this->zbxTestClickWait('gridautoalign');
		}

		$this->zbxTestClickAndAcceptAlert('sysmap_update');

		$db_map = CDBHelper::getRow('SELECT * FROM sysmaps WHERE name='.zbx_dbstr($map_name));
		$this->assertTrue(isset($db_map['sysmapid']));

		$this->assertTrue(
			$db_map['grid_size'] == substr($gridSize, 0, strpos($gridSize, 'x'))
			&& $db_map['grid_show'] == $showGrid
			&& $db_map['grid_align'] == $autoAlign
		);

		$this->zbxTestClickLinkTextWait($map_name);
		$this->zbxTestContentControlButtonClickTextWait('Edit map');

		// checking if all options remain as they were set
		$this->zbxTestDropdownAssertSelected('gridsize', $gridSize);

		if ($showGrid) {
			$this->zbxTestTextPresent('Shown');
			$this->zbxTestTextNotPresent('Hidden');
		}
		else {
			$this->zbxTestTextPresent('Hidden');
			$this->zbxTestTextNotPresent('Shown');
		}

		if ($autoAlign) {
			$this->zbxTestAssertElementText("//button[@id='gridautoalign']", 'On');
		}
		else {
			$this->zbxTestAssertElementText("//button[@id='gridautoalign']", 'Off');
		}
	}

	/**
	 * Check the screenshot of the trigger container in trigger map element.
	 */
	public function testFormMap_MapElementScreenshot() {
		// Open map "Test map 1" in edit mode.
		$this->page->login()->open('sysmap.php?sysmapid=3')->waitUntilReady();

		// Click on map element "Trigger element (CPU load)".
		$this->query('xpath://div[@data-id="5"]')->waitUntilVisible()->one()->click();

		$form = $this->query('id:map-window')->asForm()->one()->waitUntilVisible();
		$form->getField('New triggers')->selectMultiple([
				'First test trigger with tag priority',
				'Fourth test trigger with tag priority',
				'Lack of available memory'
			], 'ЗАББИКС Сервер'
		);
		$form->query('button:Add')->one()->click();

		// Take a screenshot to test draggable object position for triggers of trigger type map element.
		// TODO: screenshot should be changed after fix ZBX-22528
		$this->page->removeFocus();
		$this->assertScreenshot($this->query('id:triggerContainer')->waitUntilVisible()->one(), 'Map element');
	}
}
