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
 * @var array $data
 */

$data['form_name'] = 'host-form';
$popup_url = (new CUrl('zabbix.php'))
	->setArgument('action', 'host.edit');

if ($data['hostid'] == 0) {
	if (array_key_exists('groupids', $data) && $data['groupids']) {
		$popup_url->setArgument('groupids', $data['groupids']);
	}
	elseif ($data['clone_hostid'] !== null) {
		$popup_url->setArgument('hostid', $data['clone_hostid']);

		if ($data['clone'] === 1) {
			$popup_url->setArgument('clone', 1);
		}
	}

	$buttons = [
		[
			'title' => _('Add'),
			'class' => '',
			'keepOpen' => true,
			'isSubmit' => true,
			'action' => 'host_edit_popup.submit();'
		]
	];
}
else {
	$popup_url->setArgument('hostid', $data['hostid']);

	$buttons = [
		[
			'title' => _('Update'),
			'class' => '',
			'keepOpen' => true,
			'isSubmit' => true,
			'action' => 'host_edit_popup.submit();'
		],
		[
			'title' => _('Clone'),
			'class' => 'btn-alt',
			'keepOpen' => true,
			'isSubmit' => false,
			'action' => 'host_edit_popup.clone();'
		],
		[
			'title' => _('Delete'),
			'confirmation' => _('Delete selected host?'),
			'class' => 'btn-alt',
			'keepOpen' => true,
			'isSubmit' => false,
			'action' => 'host_edit_popup.delete('.json_encode($data['hostid']).');'
		]
	];
}

$output = [
	'header' => ($data['hostid'] == 0) ? _('New host') : _('Host'),
	'doc_url' => CDocHelper::getUrl(CDocHelper::DATA_COLLECTION_HOST_EDIT),
	'body' => (new CPartial('configuration.host.edit.html', $data))->getOutput(),
	'script_inline' => getPagePostJs().
		$this->readJsFile('popup.host.edit.js.php').
		'host_edit_popup.init('.json_encode([
			'popup_url' => $popup_url->getUrl(),
			'form_name' => $data['form_name'],
			'host_interfaces' => $data['host']['interfaces'],
			'proxy_groupid' => $data['host']['proxy_groupid'],
			'host_is_discovered' => ($data['host']['flags'] == ZBX_FLAG_DISCOVERY_CREATED),
			'warnings' => $data['warnings']
		]).');',
	'buttons' => $buttons
];

if ($data['user']['debug_mode'] == GROUP_DEBUG_MODE_ENABLED) {
	CProfiler::getInstance()->stop();
	$output['debug'] = CProfiler::getInstance()->make()->toString();
}

echo json_encode($output);
