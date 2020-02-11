<?php	declare(strict_types=1);
/*
** Zabbix
** Copyright (C) 2001-2020 Zabbix SIA
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


class CControllerAuditLogList extends CController {

	protected function checkInput(): bool {
		$fields = [
			'page' =>				'int32|ge 1',
			'auditlog_action' =>	'in -1,'.implode(',', array_keys($this->getActionsList())),
			'resourcetype' =>		'in -1,'.implode(',', array_keys($this->getResourcesList())),
			'filter_rst' =>			'in 1',
			'filter_set' =>			'in 1',
			'user_alias' =>			'string',
			'from' =>				'range_time',
			'to' =>					'range_time'
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$this->setResponse(new CControllerResponseFatal());
		}

		return $ret;
	}

	protected function checkPermissions(): bool {
		return ($this->getUserType() == USER_TYPE_SUPER_ADMIN);
	}

	protected function doAction() {
		if ($this->getInput('filter_set', 0)) {
			CProfile::update('web.auditlogs.filter.alias', $this->getInput('alias', ''), PROFILE_TYPE_STR);
			CProfile::update('web.auditlogs.filter.action', $this->getInput('action', -1), PROFILE_TYPE_INT);
			CProfile::update('web.auditlogs.filter.resourcetype', $this->getInput('resourcetype', -1), PROFILE_TYPE_INT);
		}
		elseif ($this->getInput('filter_rst', 0)) {
			CProfile::delete('web.auditlogs.filter.alias');
			CProfile::delete('web.auditlogs.filter.action');
			CProfile::delete('web.auditlogs.filter.resourcetype');
		}

		$timeselector_options = [
			'profileIdx' => 'web.auditlogs.filter',
			'profileIdx2' => 0,
			'from' => null,
			'to' => null
		];
		$this->getInputs($timeselector_options, ['from', 'to']);
		updateTimeSelectorPeriod($timeselector_options);

		$data = [
			'page' => 1,
			'user_alias' => CProfile::get('web.auditlogs.filter.alias', ''),
			'resourcetype' => CProfile::get('web.auditlogs.filter.resourcetype', -1),
			'auditlog_action' => CProfile::get('web.auditlogs.filter.action', -1),
			'action' => $this->getAction(),
			'actions' => $this->getActionsList(),
			'resources' => $this->getResourcesList(),
			'timeline' => getTimeSelectorPeriod($timeselector_options),
			'active_tab' => CProfile::get('web.auditlogs.filter.active', 1)
		];
		$this->getInputs($data, ['user_alias', 'resourcetype', 'auditlog_action', 'page']);
		$users = [];
		$filter = [];

		if (array_key_exists((int) $data['auditlog_action'], $data['actions'])) {
			$filter['action'] = $data['auditlog_action'];
		}

		if (array_key_exists((int) $data['resourcetype'], $data['resources'])) {
			$filter['resourcetype'] = $data['resourcetype'];
		}

		$config = select_config();
		$params = [
			'output' => ['auditid', 'userid', 'clock', 'action', 'resourcetype', 'note', 'ip', 'resourceid',
				'resourcename'
			],
			'selectDetails' => ['table_name', 'field_name', 'oldvalue', 'newvalue'],
			'filter' => $filter,
			'sortfield' => 'clock',
			'sortorder' => ZBX_SORT_DOWN,
			'limit' => $config['search_limit'] + 1
		];

		if ($data['timeline']['from_ts'] !== null) {
			$params['time_from'] = $data['timeline']['from_ts'];
		}

		if ($data['timeline']['to_ts'] !== null) {
			$params['time_till'] = $data['timeline']['to_ts'];
		}

		if ($data['user_alias']) {
			$users = API::User()->get([
				'output' => ['userid', 'alias'],
				'filter' => ['alias' => $data['user_alias']]
			]);

			if ($users) {
				$params['userids'] = reset($users)['userid'];
			}
		}

		$data['auditlogs'] = API::AuditLog()->get($params);
		$data['paging'] = CPagerHelper::paginate($data['page'], $data['auditlogs'], ZBX_SORT_UP,
			(new CUrl('zabbix.php'))->setArgument('action', $this->getAction())
		);

		if (!$users) {
			$users = API::User()->get([
				'output' => ['userid', 'alias'],
				'userids' => array_column($data['auditlogs'], 'userid', 'userid')
			]);
		}

		$data['users'] = array_column($users, 'alias', 'userid');

		natsort($data['actions']);
		natsort($data['resources']);

		$data['actions'] = [-1 => _('All')] + $data['actions'];
		$data['resources'] = [-1 => _('All')] + $data['resources'];

		// render view
		$response = new CControllerResponseData($data);
		$response->setTitle(_('Audit log'));
		$this->setResponse($response);
	}

	protected function init() {
		$this->disableSIDValidation();
	}

	/**
	 * Return associated list of available actions and labels.
	 *
	 * @return array
	 */
	protected function getActionsList(): array {
		return [
			AUDIT_ACTION_LOGIN => _('Login'),
			AUDIT_ACTION_LOGOUT => _('Logout'),
			AUDIT_ACTION_ADD => _('Add'),
			AUDIT_ACTION_UPDATE => _('Update'),
			AUDIT_ACTION_DELETE => _('Delete'),
			AUDIT_ACTION_ENABLE => _('Enable'),
			AUDIT_ACTION_DISABLE => _('Disable')
		];
	}

	/**
	 * Return associated list of available resources and labels.
	 *
	 * @return array
	 */
	protected function getResourcesList(): array {
		return [
			AUDIT_RESOURCE_USER => _('User'),
			AUDIT_RESOURCE_ZABBIX_CONFIG => _('Configuration of Zabbix'),
			AUDIT_RESOURCE_MEDIA_TYPE => _('Media type'),
			AUDIT_RESOURCE_HOST => _('Host'),
			AUDIT_RESOURCE_HOST_PROTOTYPE => _('Host prototype'),
			AUDIT_RESOURCE_ACTION => _('Action'),
			AUDIT_RESOURCE_GRAPH => _('Graph'),
			AUDIT_RESOURCE_GRAPH_PROTOTYPE => _('Graph prototype'),
			AUDIT_RESOURCE_GRAPH_ELEMENT => _('Graph element'),
			AUDIT_RESOURCE_USER_GROUP => _('User group'),
			AUDIT_RESOURCE_APPLICATION => _('Application'),
			AUDIT_RESOURCE_TRIGGER => _('Trigger'),
			AUDIT_RESOURCE_TRIGGER_PROTOTYPE => _('Trigger prototype'),
			AUDIT_RESOURCE_HOST_GROUP => _('Host group'),
			AUDIT_RESOURCE_ITEM => _('Item'),
			AUDIT_RESOURCE_ITEM_PROTOTYPE => _('Item prototype'),
			AUDIT_RESOURCE_IMAGE => _('Image'),
			AUDIT_RESOURCE_VALUE_MAP => _('Value map'),
			AUDIT_RESOURCE_IT_SERVICE => _('Service'),
			AUDIT_RESOURCE_MAP => _('Map'),
			AUDIT_RESOURCE_SCREEN => _('Screen'),
			AUDIT_RESOURCE_SCENARIO => _('Web scenario'),
			AUDIT_RESOURCE_DISCOVERY_RULE => _('Discovery rule'),
			AUDIT_RESOURCE_SLIDESHOW => _('Slide show'),
			AUDIT_RESOURCE_PROXY => _('Proxy'),
			AUDIT_RESOURCE_REGEXP => _('Regular expression'),
			AUDIT_RESOURCE_MAINTENANCE => _('Maintenance'),
			AUDIT_RESOURCE_SCRIPT => _('Script'),
			AUDIT_RESOURCE_MACRO => _('Macro'),
			AUDIT_RESOURCE_TEMPLATE => _('Template'),
			AUDIT_RESOURCE_ICON_MAP => _('Icon mapping'),
			AUDIT_RESOURCE_CORRELATION => _('Event correlation'),
			AUDIT_RESOURCE_DASHBOARD => _('Dashboard'),
			AUDIT_RESOURCE_AUTOREGISTRATION  => _('Autoregistration'),
			AUDIT_RESOURCE_MODULE => _('Module')
		];
	}
}
