<?php
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


require_once dirname(__FILE__).'/../include/CTest.php';

class function_DBconnect extends CTest {
	public function test_DBconnect() {
		DBclose();
		return DBconnect($error);
	}

	public function test_DBconnectIfAlreadyConnected() {
		DBclose();
		DBconnect($error);
		$this->assertFalse(DBconnect($error), 'Chuck Norris: DBconnect() must return False if the database is already opened');
	}
}
