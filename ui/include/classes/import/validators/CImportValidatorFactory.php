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


class CImportValidatorFactory extends CRegistryFactory {

	public function __construct(string $format) {
		parent::__construct([
			'1.0' => function() use ($format): CXmlValidatorGeneral {
				return new C10XmlValidator($format);
			},
			'2.0' => function() use ($format): CXmlValidatorGeneral {
				return new C20XmlValidator($format);
			},
			'3.0' => function() use ($format): CXmlValidatorGeneral {
				return new C30XmlValidator($format);
			},
			'3.2' => function() use ($format): CXmlValidatorGeneral {
				return new C32XmlValidator($format);
			},
			'3.4' => function() use ($format): CXmlValidatorGeneral {
				return new C34XmlValidator($format);
			},
			'4.0' => function() use ($format): CXmlValidatorGeneral {
				return new C40XmlValidator($format);
			},
			'4.2' => function() use ($format): CXmlValidatorGeneral {
				return new C42XmlValidator($format);
			},
			'4.4' => function() use ($format): CXmlValidatorGeneral {
				return new C44XmlValidator($format);
			},
			'5.0' => function() use ($format): CXmlValidatorGeneral {
				return new C50XmlValidator($format);
			},
			'5.2' => function() use ($format): CXmlValidatorGeneral {
				return new C52XmlValidator($format);
			},
			'5.4' => function() use ($format): CXmlValidatorGeneral {
				return new C54XmlValidator($format);
			},
			'6.0' => function() use ($format): CXmlValidatorGeneral {
				return new C60XmlValidator($format);
			},
			'6.2' => function() use ($format): CXmlValidatorGeneral {
				return new C62XmlValidator($format);
			},
			'6.4' => function() use ($format): CXmlValidatorGeneral {
				return new C64XmlValidator($format);
			},
			'7.0' => function() use ($format): CXmlValidatorGeneral {
				return new C70XmlValidator($format);
			},
			'7.2' => function() use ($format): CXmlValidatorGeneral {
				return new C72XmlValidator($format);
			}
		]);
	}
}
