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


/**
 * @var CView $this
 */
?>

<script>
	const view = {
		init({checkbox_hash, checkbox_object, context, parent_discoveryid, form_name}) {
			this.checkbox_hash = checkbox_hash;
			this.checkbox_object = checkbox_object;
			this.context = context;
			this.is_discovery = parent_discoveryid !== null;
			this.form = document.forms[form_name];

			this._initActions();
		},

		_initActions() {
			const copy = document.querySelector('.js-copy');

			if (copy !== null) {
				copy.addEventListener('click', () => {
					const overlay = this.openCopyPopup();
					const dialogue = overlay.$dialogue[0];

					dialogue.addEventListener('dialogue.submit', (e) => {
						postMessageOk(e.detail.success.title);

						const uncheckids = Object.keys(chkbxRange.getSelectedIds());
						uncheckTableRows('graphs_' + this.checkbox_hash, [], false);
						chkbxRange.checkObjects(this.checkbox_object, uncheckids, false);
						chkbxRange.update(this.checkbox_object);

						if ('messages' in e.detail.success) {
							postMessageDetails('success', e.detail.success.messages);
						}

						location.href = location.href;
					});
				});
			}

			this.form.addEventListener('click', (e) => {
				const target = e.target;

				if (target.classList.contains('js-edit-host')) {
					this.editHost(e, target.dataset.hostid);
				}
				else if (target.classList.contains('js-edit-template')) {
					this.editTemplate(e, target.dataset.hostid);
				}
			});
		},

		openCopyPopup() {
			const parameters = {
				graphids: Object.keys(chkbxRange.getSelectedIds()),
				source: 'graphs'
			};

			const filter_hostids = document.getElementsByName('filter_hostids[]');
			const context = document.getElementById('context');

			if (filter_hostids.length == 1) {
				parameters.src_hostid = context === 'host' ? filter_hostids[0].value : 0;
			}

			return PopUp('copy.edit', parameters, {
				dialogueid: 'copy',
				dialogue_class: 'modal-popup-static'
			});
		},

		editHost(e, hostid) {
			e.preventDefault();
			const host_data = {hostid};

			this.openHostPopup(host_data);
		},

		editTemplate(e, templateid) {
			e.preventDefault();
			const template_data = {templateid};

			this.openTemplatePopup(template_data);
		},

		openHostPopup(host_data) {
			const original_url = location.href;
			const overlay = PopUp('popup.host.edit', host_data, {
				dialogueid: 'host_edit',
				dialogue_class: 'modal-popup-large',
				prevent_navigation: true
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit',
				this.events.elementSuccess.bind(this, this.context, this.is_discovery), {once: true}
			);
			overlay.$dialogue[0].addEventListener('dialogue.close', () => {
				history.replaceState({}, '', original_url);
			}, {once: true});
		},

		openTemplatePopup(template_data) {
			const overlay =  PopUp('template.edit', template_data, {
				dialogueid: 'templates-form',
				dialogue_class: 'modal-popup-large',
				prevent_navigation: true
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit',
				this.events.elementSuccess.bind(this, this.context, this.is_discovery), {once: true}
			);
		},

		events: {
			elementSuccess(context, discovery, e) {
				const data = e.detail;
				let curl = null;

				if ('success' in data) {
					postMessageOk(data.success.title);

					if ('messages' in data.success) {
						postMessageDetails('success', data.success.messages);
					}

					if ('action' in data.success && data.success.action === 'delete') {
						curl = discovery ? new Curl('host_discovery.php') : new Curl('graphs.php');
						curl.setArgument('context', context);
					}
				}

				uncheckTableRows('graphs_' + this.checkbox_hash, [], false);

				location.href = curl === null? location.href : curl.getUrl();
			}
		}
	};
</script>
