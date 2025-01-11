<?php declare(strict_types = 0);
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
?>


window.widget_slareport_form = new class {

	init({serviceid_field_id}) {
		this._$service = jQuery(`#${serviceid_field_id}`);
		this._$service.multiSelect('getSelectButton').addEventListener('click', () => this.selectService());
	}

	selectService() {
		const exclude_serviceids = [];

		for (const service of this._$service.multiSelect('getData')) {
			exclude_serviceids.push(service.id);
		}

		const overlay = PopUp('popup.services', {
			title: <?= json_encode(_('Service')) ?>,
			exclude_serviceids,
			multiple: 0
		}, {dialogueid: 'services'});

		overlay.$dialogue[0].addEventListener('dialogue.submit', (e) => {
			const data = [];

			for (const service of e.detail) {
				data.push({id: service.serviceid, name: service.name});
			}

			this._$service.multiSelect('addData', data);
		});
	}
};
