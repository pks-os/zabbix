<?php
/*
** Zabbix
** Copyright (C) 2001-2023 Zabbix SIA
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


/**
 * @var CView $this
 */
?>

window.check_popup = new class {

	init({dcheckid}) {
		this.overlay = overlays_stack.getById('discovery-check');
		this.dialogue = this.overlay.$dialogue[0];
		this.form = this.overlay.$dialogue.$body[0].querySelector('form');
		this.dcheckid = dcheckid;

		this._loadViews();
	}

	_loadViews() {
		new CViewSwitcher('type-select', 'change', <?= json_encode([
			SVC_SSH => ['row_dcheck_ports'],
			SVC_LDAP => ['row_dcheck_ports'],
			SVC_SMTP => ['row_dcheck_ports'],
			SVC_FTP => ['row_dcheck_ports'],
			SVC_HTTP => ['row_dcheck_ports'],
			SVC_POP => ['row_dcheck_ports'],
			SVC_NNTP => ['row_dcheck_ports'],
			SVC_IMAP => ['row_dcheck_ports'],
			SVC_TCP => ['row_dcheck_ports'],
			SVC_AGENT => ['row_dcheck_ports', 'row_dcheck_key'],
			SVC_SNMPv1 => ['row_dcheck_ports', 'row_dcheck_snmp_community', 'row_dcheck_snmp_oid'],
			SVC_SNMPv2c => ['row_dcheck_ports', 'row_dcheck_snmp_community', 'row_dcheck_snmp_oid'],
			SVC_ICMPPING => [],
			SVC_SNMPv3 => ['row_dcheck_ports', 'row_dcheck_snmp_oid', 'row_dcheck_snmpv3_contextname',
				'row_dcheck_snmpv3_securityname', 'row_dcheck_snmpv3_securitylevel', 'row_dcheck_snmpv3_authprotocol',
				'row_dcheck_snmpv3_authpassphrase', 'row_dcheck_snmpv3_privprotocol', 'row_dcheck_snmpv3_privpassphrase'
			],
			SVC_HTTPS => ['row_dcheck_ports'],
			SVC_TELNET => ['row_dcheck_ports']
		], JSON_THROW_ON_ERROR) ?>);

		// todo - rewrite jqueries to vanilla js

		var $type = jQuery('#type-select'),
			$snmpv3_securitylevel = jQuery('#snmpv3-securitylevel');

		$type.on('change', function() {
			$snmpv3_securitylevel.off('change');

			if (jQuery(this).val() == <?= SVC_SNMPv3 ?>) {
				new CViewSwitcher('snmpv3-securitylevel', 'change', <?= json_encode([
					ITEM_SNMPV3_SECURITYLEVEL_NOAUTHNOPRIV => [],
					ITEM_SNMPV3_SECURITYLEVEL_AUTHNOPRIV => ['row_dcheck_snmpv3_authprotocol',
						'row_dcheck_snmpv3_authpassphrase'
					],
					ITEM_SNMPV3_SECURITYLEVEL_AUTHPRIV => ['row_dcheck_snmpv3_authprotocol', 'row_dcheck_snmpv3_authpassphrase',
						'row_dcheck_snmpv3_privprotocol', 'row_dcheck_snmpv3_privpassphrase'
					]
				], JSON_THROW_ON_ERROR) ?>);

				$snmpv3_securitylevel.on('change', function() {
					jQuery(window).trigger('resize');
				});
			}

			jQuery(window).trigger('resize');
		});

		let that = this;

		if ($type.val() == <?= SVC_SNMPv3 ?>) {
			// Fires the change event to initialize CViewSwitcher.
			$type.trigger('change');

			// Now we can add the event to clear the form on type change.
			$type.on('change', function() {
				that._clearDCheckForm();
				that._setDCheckDefaultPort();
			});
		}
		else {
			$type.on('change', function() {
				that._clearDCheckForm();
				that._setDCheckDefaultPort();
			});
		}
	}

	/**
	 * Get check table data to check if row with the same data already exists.
	 */
	_addCheckTableData() {
		// todo - fix and pass all hiddden input from row
		const table = document.getElementById('dcheckList');
		const tbody = table.getElementsByTagName('tbody')[0];
		const trList = tbody.getElementsByTagName('tr');
		let dchecks = [];

		[...trList].map(element => {
			const table_row = element.getElementsByTagName('td')

			if (table_row.length > 0) {
				dchecks.push(table_row[0].innerHTML)
			}
		});

		return dchecks;
	}

	/**
	 * Checks duplicate discovery cheks.
	 */
	_hasDCheckDuplicates() {
		const dcheck = getFormFields(this.form);

		let fields = [
			'dcheckid', 'type', 'ports', 'snmp_community', 'key_', 'snmpv3_contextname', 'snmpv3_securityname',
			'snmpv3_securitylevel', 'snmpv3_authprotocol', 'snmpv3_authpassphrase', 'snmpv3_privprotocol',
			'snmpv3_privpassphrase'
		];
		let result = [];
		let duplicate = [];
		let has_duplicates;

		[...document.getElementById('dcheckList').getElementsByTagName('td')].map(element => {
			let inputs = element.querySelectorAll('input');

			inputs.forEach(input => {
				for (let i = 0; i < fields.length; i++) {
					if (input.name.includes(fields[i])) {
						result[fields[i]] = input.value;
						break;
					}
				}
			});

			let keys = ['snmpv3_securitylevel', 'snmpv3_authprotocol', 'snmpv3_privprotocol'];
			let duplicates = [];

			if (dcheck['type'] == <?= SVC_SNMPv1 ?> || dcheck['type'] == <?= SVC_SNMPv2c ?>
				|| dcheck['type'] == <?= SVC_SNMPv3 ?>) {
				dcheck['key_'] = dcheck['snmp_oid'];
			}

			for (const key in result) {
				if (dcheck.type !== 13 && keys.includes(key)) {
					delete result[key];
				}

				if (dcheck.hasOwnProperty(key) && result[key] === dcheck[key]) {
					duplicates.push(key);
				}

				// remove data no dcheck update to not compare dcheck to itself
				if (key == 'dcheckid' && result[key] === dcheck[key]) {
					result = [];
				}

				if (key === 'dcheckid') {
					delete result[key];

					if (dcheck.hasOwnProperty(key) && result[key] === dcheck[key]) {
						duplicates.push(key);
					}
				}
			}

			duplicate.push(duplicates.length > 0 && duplicates.length === Object.keys(result).length);
		});

		duplicate.forEach((result) => {
			if (result === true) {
				has_duplicates = true;
			}
		})

		return has_duplicates;
	}



	submit() {
		for (const element of this.form.parentNode.children) {
			if (element.matches('.msg-good, .msg-bad, .msg-warning')) {
				element.parentNode.removeChild(element);
			}
		}

		if (this._hasDCheckDuplicates()) {
			jQuery(makeMessageBox('bad', [<?= json_encode(_('Check already exists.')) ?>]))
				.insertBefore(this.form);

			this.overlay.unsetLoading();
		}

		else {
			const curl = new Curl('zabbix.php');
			const fields = getFormFields(this.form);

			for (const key in fields) {
				if (fields.hasOwnProperty(key) && fields[key].trim() === '') {
					delete fields[key];
				}

				if (fields.hasOwnProperty(key) && ['name', 'iprange', 'delay'].includes(key)) {
					fields[key] = fields[key].trim()
				}
			}

			curl.setArgument('action', 'discovery.check.check');
			this._post(curl.getUrl(), fields);
		}
	}

	_post(url, data) {
		fetch(url, {
			method: 'POST',
			headers: {'Content-Type': 'application/json'},
			body: JSON.stringify(data)
		})
			.then((response) => response.json())
			.then((response) => {
				if ('error' in response) {
					throw {error: response.error};
				}

				this.dialogue.dispatchEvent(new CustomEvent('check.submit', {detail: response}));
				overlayDialogueDestroy(this.overlay.dialogueid);
			})
			.catch((exception) => {
				for (const element of this.form.parentNode.children) {
					if (element.matches('.msg-good, .msg-bad, .msg-warning')) {
						element.parentNode.removeChild(element);
					}
				}

				let title, messages;

				if (typeof exception === 'object' && 'error' in exception) {
					title = exception.error.title;
					messages = exception.error.messages;
				}
				else {
					messages = [<?= json_encode(_('Unexpected server error.')) ?>];
				}

				const message_box = makeMessageBox('bad', messages, title)[0];
				this.form.parentNode.insertBefore(message_box, this.form);
			})
			.finally(() => {
				this.overlay.unsetLoading();
			});
	}

	/**
	 * Resets fields of the discovery check form to default values.
	 */
	_clearDCheckForm() {
		const elementsToClear = document.querySelectorAll('#key_, #snmp_community, #snmp_oid, #snmpv3_contextname, #snmpv3_securityname, #snmpv3_authpassphrase, #snmpv3_privpassphrase');
		elementsToClear.forEach(function(element) {
			element.value = '';
		});

		if (document.querySelector('#snmpv3-securitylevel')) {
			document.querySelector('#snmpv3-securitylevel').value = <?= ITEM_SNMPV3_SECURITYLEVEL_NOAUTHNOPRIV ?>;
		}

		if (document.querySelector('#snmpv3_authprotocol_0')) {
			document.querySelector('#snmpv3_authprotocol_0').checked = true;
		}

		if (document.querySelector('#snmpv3_privprotocol_0')) {
			document.querySelector('#snmpv3_privprotocol_0').checked = true;
		}
	}

	/**
	 * Set default discovery check port to input.
	 */
	_setDCheckDefaultPort() {
		document.querySelector('#ports').value = this._getDCheckDefaultPort(
			document.querySelector('#type-select').value
		);
	}

	/**
	 * Returns a default port number for the specified discovery check type.
	 *
	 * @param {string} dcheck_type  Discovery check type.
	 *
	 * @returns {string}
	 */
	_getDCheckDefaultPort(dcheck_type) {
		var default_ports = {
			<?= SVC_SSH ?>: '22',
			<?= SVC_LDAP ?>: '389',
			<?= SVC_SMTP ?>: '25',
			<?= SVC_FTP ?>:  '21',
			<?= SVC_HTTP ?>: '80',
			<?= SVC_POP ?>: '110',
			<?= SVC_NNTP ?>: '119',
			<?= SVC_IMAP ?>: '143',
			<?= SVC_AGENT ?>: '10050',
			<?= SVC_SNMPv1 ?>: '161',
			<?= SVC_SNMPv2c ?>: '161',
			<?= SVC_SNMPv3 ?>: '161',
			<?= SVC_HTTPS ?>: '443',
			<?= SVC_TELNET ?>: '23'
		};

		return default_ports.hasOwnProperty(dcheck_type) ? default_ports[dcheck_type] : '0';
	}
}
