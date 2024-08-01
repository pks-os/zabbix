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


/**
 * @var CView $this
 */
?>

<script type="text/x-jquery-tmpl" id="filter-tag-row-tmpl">
	<?= CTagFilterFieldHelper::getTemplate() ?>
</script>

<script>
	const view = {
		applied_filter_groupids: [],

		init({applied_filter_groupids}) {
			this.applied_filter_groupids = applied_filter_groupids;

			this.initFilter();

			document.addEventListener('click', (e) => {
				if (e.target.classList.contains('js-edit-template')) {
					this.editTemplate({templateid: e.target.dataset.templateid});
				}
				else if (e.target.classList.contains('js-edit-proxy')) {
					this.editProxy(e.target.dataset.proxyid);
				}
				else if (e.target.classList.contains('js-edit-proxy-group')) {
					this.editProxyGroup(e.target.dataset.proxy_groupid);
				}
			});
		},

		editTemplate(parameters) {
			const overlay = PopUp('template.edit', parameters, {
				dialogueid: 'templates-form',
				dialogue_class: 'modal-popup-large',
				prevent_navigation: true
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit', (e) => this.reload(e.detail.success));
		},

		editProxy(proxyid) {
			const overlay = PopUp('popup.proxy.edit', {proxyid}, {
				dialogueid: 'proxy_edit',
				dialogue_class: 'modal-popup-static',
				prevent_navigation: true
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit', (e) => this.reload(e.detail.success));
		},

		editProxyGroup(proxy_groupid) {
			const overlay = PopUp('popup.proxygroup.edit', {proxy_groupid}, {
				dialogueid: 'proxy-group-edit',
				dialogue_class: 'modal-popup-static',
				prevent_navigation: true
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit', (e) => this.reload(e.detail.success));
		},

		reload(success) {
			postMessageOk(success.title);

			if ('messages' in success) {
				postMessageDetails('success', success.messages);
			}

			uncheckTableRows('hosts');

			location.href = location.href;
		},

		initFilter() {
			$('#filter-tags')
				.dynamicRows({template: '#filter-tag-row-tmpl'})
				.on('afteradd.dynamicRows', function () {
					const rows = this.querySelectorAll('.form_row');
					new CTagFilterItem(rows[rows.length - 1]);
				});

			// Init existing fields once loaded.
			document.querySelectorAll('#filter-tags .form_row').forEach(row => {
				new CTagFilterItem(row);
			});

			$('#filter_monitored_by')
				.on('change', function() {
					const filter_monitored_by = $('input[name=filter_monitored_by]:checked').val();

					for (const field of document.querySelectorAll('.js-filter-proxyids')) {
						field.style.display = filter_monitored_by == <?= ZBX_MONITORED_BY_PROXY ?> ? '' : 'none';
					}

					$('#filter_proxyids_').multiSelect(
						filter_monitored_by == <?= ZBX_MONITORED_BY_PROXY ?> ? 'enable' : 'disable'
					);

					for (const field of document.querySelectorAll('.js-filter-proxy-groupids')) {
						field.style.display = filter_monitored_by == <?= ZBX_MONITORED_BY_PROXY_GROUP ?> ? '' : 'none';
					}

					$('#filter_proxy_groupids_').multiSelect(
						filter_monitored_by == <?= ZBX_MONITORED_BY_PROXY_GROUP ?> ? 'enable' : 'disable'
					);
				})
				.trigger('change');
		},

		createHost() {
			const host_data = this.applied_filter_groupids
				? {groupids: this.applied_filter_groupids}
				: {};

			this.openHostPopup(host_data);
		},

		editHost(e, hostid) {
			e.preventDefault();
			const host_data = {hostid};

			this.openHostPopup(host_data);
		},

		openHostPopup(host_data) {
			const original_url = location.href;
			const overlay = PopUp('popup.host.edit', host_data, {
				dialogueid: 'host_edit',
				dialogue_class: 'modal-popup-large',
				prevent_navigation: true
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit', this.events.elementSuccess, {once: true});
			overlay.$dialogue[0].addEventListener('dialogue.close', () => {
				history.replaceState({}, '', original_url);
			}, {once: true});
		},

		massDeleteHosts(button) {
			const confirm_text = Object.keys(chkbxRange.getSelectedIds()).length > 1
				? <?= json_encode(_('Delete selected hosts?')) ?>
				: <?= json_encode(_('Delete selected host?')) ?>;

			if (!confirm(confirm_text)) {
				return;
			}

			button.classList.add('is-loading');

			const curl = new Curl('zabbix.php');
			curl.setArgument('action', 'host.massdelete');
			curl.setArgument(CSRF_TOKEN_NAME, <?= json_encode(CCsrfTokenHelper::get('host')) ?>);

			fetch(curl.getUrl(), {
				method: 'POST',
				headers: {'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8'},
				body: urlEncodeData({hostids: Object.keys(chkbxRange.getSelectedIds())})
			})
				.then((response) => response.json())
				.then((response) => {
					if ('error' in response) {
						if ('title' in response.error) {
							postMessageError(response.error.title);
						}

						postMessageDetails('error', response.error.messages);

						uncheckTableRows('hosts', response.keepids ?? []);
					}
					else if ('success' in response) {
						postMessageOk(response.success.title);

						if ('messages' in response.success) {
							postMessageDetails('success', response.success.messages);
						}

						uncheckTableRows('hosts');
					}

					location.href = location.href;
				})
				.catch(() => {
					clearMessages();

					const message_box = makeMessageBox('bad', [<?= json_encode(_('Unexpected server error.')) ?>]);

					addMessage(message_box);
				})
				.finally(() => {
					button.classList.remove('is-loading');
				});
		},

		events: {
			elementSuccess(e) {
				const data = e.detail;

				if ('success' in data) {
					postMessageOk(data.success.title);

					if ('messages' in data.success) {
						postMessageDetails('success', data.success.messages);
					}
				}

				uncheckTableRows('hosts');
				location.href = location.href;
			}
		}
	};
</script>
