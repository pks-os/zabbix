<?php declare(strict_types=0);
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


class CControllerItemPrototypeDisable extends CControllerItemPrototype {

	protected function init(): void {
		$this->setPostContentType(self::POST_CONTENT_TYPE_JSON);
	}

	protected function checkInput(): bool {
		$fields = [
			'context'	=> 'required|in host,template',
			'field'		=> 'required|in status,discover',
			'itemids'	=> 'required|array_id'
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$this->setResponse(
				new CControllerResponseData(['main_block' => json_encode([
					'error' => [
						'messages' => array_column(get_and_clear_messages(), 'message')
					]
				])])
			);
		}

		return $ret;
	}

	public function doAction() {
		$output = [];
		$items = [];
		$fields = $this->getInput('field') === 'status'
			? ['status' => ITEM_STATUS_DISABLED]
			: ['discover' => ITEM_NO_DISCOVER];

		foreach ($this->getInput('itemids') as $itemid) {
			$items[] = ['itemid' => $itemid] + $fields;
		}

		$result = API::ItemPrototype()->update($items);
		$messages = array_column(get_and_clear_messages(), 'message');
		$count = count($items);

		if ($result) {
			$output['success']['title'] = _n('Item prototype updated', 'Item prototypes updated', $count);

			if ($messages) {
				$output['success']['messages'] = $messages;
			}
		}
		else {
			$output['error'] = [
				'title' => _n('Cannot update item prototype', 'Cannot update item prototypes', $count),
				'messages' => $messages
			];
		}

		$this->setResponse(new CControllerResponseData(['main_block' => json_encode($output)]));
	}
}
