<?php declare(strict_types = 0);
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


class CSubmitButtonTest extends CTagTest {

	public function constructProvider() {
		return [
			[
				['caption'],
				'<button type="submit" tabindex="0">caption</button>'
			],
			[
				['caption', 'button'],
				'<button type="submit" tabindex="0" name="button">caption</button>'
			],
			[
				['caption', 'button[value]'],
				'<button type="submit" tabindex="0" name="button[value]">caption</button>'
			],
			[
				['caption', 'button', 'value'],
				'<button type="submit" tabindex="0" name="button" value="value">caption</button>'
			],
			// caption encoding
			[
				['</button>'],
				'<button type="submit" tabindex="0">&lt;/button&gt;</button>'
			],
			// parameter encoding
			[
				['caption', 'button', 'button"&"'],
				'<button type="submit" tabindex="0" name="button" value="button&quot;&amp;&quot;">caption</button>'
			]
		];
	}
	/**
	 * @param $name
	 * @param $value
	 * @param $caption
	 *
	 * @return CSubmitButton
	 */
	protected function createTag($caption = null, $name = null, $value = null) {
		return new CSubmitButton($caption, $name, $value);
	}
}
