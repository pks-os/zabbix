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
 * Top hosts widget view.
 *
 * @var CView $this
 * @var array $data
 */

use Widgets\TopHosts\Widget;

use Widgets\TopHosts\Includes\CWidgetFieldColumnsList;

$table = (new CTableInfo())->addClass($data['show_thumbnail'] ? 'show-thumbnail' : null);

if ($data['error'] !== null) {
	$table->setNoDataMessage($data['error']);
}
else {
	$header = [];

	foreach ($data['configuration'] as $column_config) {
		if ($column_config['data'] == CWidgetFieldColumnsList::DATA_ITEM_VALUE
				&& $column_config['display_item_as'] == CWidgetFieldColumnsList::DISPLAY_VALUE_AS_NUMERIC
				&& $column_config['display'] != CWidgetFieldColumnsList::DISPLAY_AS_IS) {
			$header[] = (new CColHeader(
				(new CSpan($column_config['name']))
					->setTitle($column_config['name'])
			))->setColSpan(2);
		}
		else {
			$header[] = new CColHeader($column_config['name']);
		}
	}

	$table->setHeader($header);

	foreach ($data['rows'] as ['columns' => $columns, 'context' => $context]) {
		$row = [];

		foreach ($columns as $i => $column) {
			$column_config = $data['configuration'][$i];

			if ($column === null) {
				if ($column_config['data'] == CWidgetFieldColumnsList::DATA_ITEM_VALUE
						&& $column_config['display_item_as'] == CWidgetFieldColumnsList::DISPLAY_VALUE_AS_NUMERIC
						&& $column_config['display'] != CWidgetFieldColumnsList::DISPLAY_AS_IS) {
					$row[] = (new CCol(''))
						->setColSpan(2)
						->setAttribute('no-data', true);
				}
				else {
					$row[] = '';
				}

				continue;
			}

			$color = $column_config['base_color'];

			if ($column_config['data'] == CWidgetFieldColumnsList::DATA_ITEM_VALUE
					&& $column_config['display'] == CWidgetFieldColumnsList::DISPLAY_AS_IS
					&& array_key_exists('thresholds', $column_config)) {
				$is_numeric_data = in_array($column['item']['value_type'],
					[ITEM_VALUE_TYPE_FLOAT, ITEM_VALUE_TYPE_UINT64]
				) || CAggFunctionData::isNumericResult($column_config['aggregate_function']);

				if ($is_numeric_data) {
					foreach ($column_config['thresholds'] as $threshold) {
						$threshold_value = $column['is_binary_units']
							? $threshold['threshold_binary']
							: $threshold['threshold'];

						if ($column['value'] < $threshold_value) {
							break;
						}

						$color = $threshold['color'];
					}
				}
			}

			switch ($column_config['data']) {
				case CWidgetFieldColumnsList::DATA_HOST_NAME:
					$row[] = (new CCol(
						(new CLinkAction($column['value']))->setMenuPopup(CMenuPopupHelper::getHost($column['hostid']))
					))
						->addStyle($color !== '' ? 'background-color: #' . $color : null)
						->addItem($column['maintenance_status'] == HOST_MAINTENANCE_STATUS_ON
							? makeMaintenanceIcon($column['maintenance_type'], $column['maintenance_name'],
								$column['maintenance_description']
							)
							: null
						);

					break;

				case CWidgetFieldColumnsList::DATA_TEXT:
					$row[] = (new CCol($column['value']))
						->addStyle($color !== '' ? 'background-color: #' . $color : null);

					break;

				case CWidgetFieldColumnsList::DATA_ITEM_VALUE:
					if ($column['item']['value_type'] == ITEM_VALUE_TYPE_BINARY) {
						$formatted_value = italic(_('binary value'))->addClass($color === '' ? ZBX_STYLE_GREY : null);
						$column['value'] = _('binary value');
					}
					else {
						$formatted_value = formatAggregatedHistoryValue($column['value'], $column['item'],
							$column_config['aggregate_function'], false, true, [
								'decimals' => $column_config['decimal_places'],
								'decimals_exact' => true,
								'small_scientific' => false,
								'zero_as_zero' => false
							]
						);
					}

					if ($column_config['display'] == CWidgetFieldColumnsList::DISPLAY_AS_IS) {
						$row[] = (new CCol())
							->addStyle($color !== '' ? 'background-color: #' . $color : null)
							->addItem(
								(new CDiv($formatted_value))
									->addClass(ZBX_STYLE_CURSOR_POINTER)
									->setHint(
										(new CDiv($column['value']))->addClass(ZBX_STYLE_HINTBOX_WRAP)
									)
							);

						break;
					}

					$bar_gauge = (new CBarGauge())
						->setValue($column['value'])
						->setAttribute('fill', $column_config['base_color'] !== ''
							? '#' . $column_config['base_color']
							: Widget::DEFAULT_FILL
						)
						->setAttribute('min', $column['is_binary_units']
							? $column_config['min_binary']
							: $column_config['min']
						)
						->setAttribute('max', $column['is_binary_units']
							? $column_config['max_binary']
							: $column_config['max']
						);

					if ($column_config['display'] == CWidgetFieldColumnsList::DISPLAY_BAR) {
						$bar_gauge->setAttribute('solid', 1);
					}

					if (array_key_exists('thresholds', $column_config)) {
						foreach ($column_config['thresholds'] as $threshold) {
							$threshold_value = $column['is_binary_units']
								? $threshold['threshold_binary']
								: $threshold['threshold'];

							$bar_gauge->addThreshold($threshold_value, '#'.$threshold['color']);
						}
					}

					$row[] = new CCol($bar_gauge);
					$row[] = (new CCol())
						->addStyle('width: 0;')
						->addItem(
							(new CDiv($formatted_value))
								->addClass(ZBX_STYLE_CURSOR_POINTER)
								->addClass(ZBX_STYLE_NOWRAP)
								->setHint(
									(new CDiv($column['value']))->addClass(ZBX_STYLE_HINTBOX_WRAP)
								)
						);

					break;
			}
		}

		$is_empty_row = true;

		foreach ($row as $column) {
			if ($column === '') {
				continue;
			}

			if ($column instanceof CCol) {
				$attributes = $column->attributes ?? [];
				if (!isset($attributes['no-data'])) {
					$is_empty_row = false;
					break;
				}
			}
			else {
				$is_empty_row = false;
				break;
			}
		}

		if (!$is_empty_row) {
			$table->addRow(
				(new CRow($row))->setAttribute('data-hostid', $context['hostid'])
			);
		}
	}
}

(new CWidgetView($data))
	->addItem($table)
	->show();
