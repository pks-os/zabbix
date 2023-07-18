<?php declare(strict_types = 0);
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


class CControllerTriggerPrototypeEnable extends CController {

	protected function init(): void {
		$this->setPostContentType(self::POST_CONTENT_TYPE_JSON);
	}

	protected function checkInput(): bool {
		$fields = [
			'triggerids' =>		'required|array_db triggers.triggerid',
			'discover' =>		'db triggers.discover|in '.ZBX_PROTOTYPE_DISCOVER,
			'status' =>			'db triggers.status|in '.TRIGGER_STATUS_ENABLED
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$this->setResponse(
				(new CControllerResponseData(['main_block' => json_encode([
					'error' => [
						'messages' => array_column(get_and_clear_messages(), 'message')
					]
				])]))->disableView()
			);
		}

		return $ret;
	}

	protected function checkPermissions(): bool {
		$triggerids = $this->getInput('triggerids');

		return count($triggerids) == API::TriggerPrototype()->get([
				'countOutput' => true,
				'triggerids' => $triggerids
			]);
	}

	protected function doAction(): void {
		$triggerids = $this->getInput('triggerids');
		$triggers_count = count($triggerids);
		$triggers = [];

		if ($this->hasInput('status')) {
			foreach ($triggerids as $triggerid) {
				$triggers[] = ['triggerid' => $triggerid, 'status' => TRIGGER_STATUS_ENABLED];
			}
		}
		else {
			foreach ($triggerids as $triggerid) {
				$triggers[] = ['triggerid' => $triggerid, 'discover' => TRIGGER_DISCOVER];
			}
		}

		$result = API::TriggerPrototype()->update($triggers);

		$output = [];

		if ($result) {
			$output['success']['title'] = _n('Trigger prototype updated', 'Trigger prototypes updated',
				$triggers_count
			);

			if ($messages = get_and_clear_messages()) {
				$output['success']['messages'] = array_column($messages, 'message');
			}
		}
		else {
			$output['error'] = [
				'title' => _n('Cannot update trigger prototype', 'Cannot update trigger prototypes', $triggers_count),
				'messages' => array_column(get_and_clear_messages(), 'message')
			];

			$triggers = API::TriggerPrototype()->get([
				'output' => [],
				'triggerids' => $triggerids,
				'preservekeys' => true
			]);

			$output['keepids'] = array_keys($triggers);
		}

		$this->setResponse(new CControllerResponseData(['main_block' => json_encode($output)]));
	}
}
