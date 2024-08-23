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


class CControllerMediatypeList extends CController {

	protected function init(): void {
		$this->disableCsrfValidation();
	}

	protected function checkInput(): bool {
		$fields = [
			'sort' =>			'in name,type',
			'sortorder' =>		'in '.ZBX_SORT_DOWN.','.ZBX_SORT_UP,
			'filter_set' =>		'in 1',
			'filter_rst' =>		'in 1',
			'filter_name' =>	'string',
			'filter_status' =>	'in -1,'.MEDIA_TYPE_STATUS_ACTIVE.','.MEDIA_TYPE_STATUS_DISABLED,
			'page' =>			'ge 1'
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$this->setResponse(new CControllerResponseFatal());
		}

		return $ret;
	}

	protected function checkPermissions(): bool {
		return $this->checkAccess(CRoleHelper::UI_ADMINISTRATION_MEDIA_TYPES);
	}

	protected function doAction(): void {
		$sort_field = $this->getInput('sort', CProfile::get('web.media_types.php.sort', 'name'));
		$sort_order = $this->getInput('sortorder', CProfile::get('web.media_types.php.sortorder', ZBX_SORT_UP));
		CProfile::update('web.media_types.php.sort', $sort_field, PROFILE_TYPE_STR);
		CProfile::update('web.media_types.php.sortorder', $sort_order, PROFILE_TYPE_STR);

		// filter
		if ($this->hasInput('filter_set')) {
			CProfile::update('web.media_types.filter_name', $this->getInput('filter_name', ''), PROFILE_TYPE_STR);
			CProfile::update('web.media_types.filter_status', $this->getInput('filter_status', -1), PROFILE_TYPE_INT);
		}
		elseif ($this->hasInput('filter_rst')) {
			CProfile::delete('web.media_types.filter_name');
			CProfile::delete('web.media_types.filter_status');
		}

		$filter = [
			'name' => CProfile::get('web.media_types.filter_name', ''),
			'status' => CProfile::get('web.media_types.filter_status', -1)
		];

		$data = [
			'sort' => $sort_field,
			'sortorder' => $sort_order,
			'filter' => $filter,
			'profileIdx' => 'web.media_types.filter',
			'active_tab' => CProfile::get('web.media_types.filter.active', 1)
		];

		$limit = CSettingsHelper::get(CSettingsHelper::SEARCH_LIMIT) + 1;
		$data['mediatypes'] = API::Mediatype()->get([
			'output' => ['mediatypeid', 'name', 'type', 'smtp_server', 'smtp_helo', 'smtp_email', 'exec_path',
				'gsm_modem', 'username', 'status', 'provider'
			],
			'search' => [
				'name' => $filter['name'] === '' ? null : $filter['name']
			],
			'filter' => [
				'status' => $filter['status'] == -1 ? null : $filter['status']
			],
			'selectActions' => ['actionid', 'name', 'eventsource'],
			'limit' => $limit,
			'preservekeys' => true
		]);

		if ($data['mediatypes']) {
			$access_to_actions = [
				EVENT_SOURCE_TRIGGERS => $this->checkAccess(CRoleHelper::UI_CONFIGURATION_TRIGGER_ACTIONS),
				EVENT_SOURCE_DISCOVERY => $this->checkAccess(CRoleHelper::UI_CONFIGURATION_DISCOVERY_ACTIONS),
				EVENT_SOURCE_AUTOREGISTRATION => $this->checkAccess(
					CRoleHelper::UI_CONFIGURATION_AUTOREGISTRATION_ACTIONS
				),
				EVENT_SOURCE_INTERNAL => $this->checkAccess(CRoleHelper::UI_CONFIGURATION_INTERNAL_ACTIONS),
				EVENT_SOURCE_SERVICE => $this->checkAccess(CRoleHelper::UI_CONFIGURATION_SERVICE_ACTIONS)
			];

			foreach ($data['mediatypes'] as &$media_type) {
				$media_type['typeid'] = $media_type['type'];
				$media_type['type'] = CMediatypeHelper::getMediaTypes($media_type['type']);
				$media_type['action_count_total'] = count($media_type['actions']);
				$media_type['actions'] = array_slice($media_type['actions'], 0,
					CSettingsHelper::get(CSettingsHelper::MAX_IN_TABLE)
				);

				foreach ($media_type['actions'] as &$action) {
					$action['is_editable'] = $access_to_actions[$action['eventsource']];
				}
				unset($action);
			}
			unset($media_type);

			CArrayHelper::sort($data['mediatypes'], [['field' => $sort_field, 'order' => $sort_order]]);
		}

		// pager
		$data['page'] = $this->getInput('page', 1);
		CPagerHelper::savePage('mediatype.list', $data['page']);
		$data['paging'] = CPagerHelper::paginate($data['page'], $data['mediatypes'], $sort_order,
			(new CUrl('zabbix.php'))->setArgument('action', $this->getAction())
		);

		$response = new CControllerResponseData($data);
		$response->setTitle(_('Configuration of media types'));
		$this->setResponse($response);
	}
}
