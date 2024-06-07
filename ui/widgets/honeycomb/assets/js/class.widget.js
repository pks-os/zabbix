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


class CWidgetHoneycomb extends CWidget {

	static ZBX_STYLE_DASHBOARD_WIDGET_PADDING_V = 8;
	static ZBX_STYLE_DASHBOARD_WIDGET_PADDING_H = 10;

	/**
	 * @type {CSVGHoneycomb|null}
	 */
	#honeycomb = null;

	/**
	 * @type {boolean}
	 */
	#user_interacting = false;

	/**
	 * @type {number}
	 */
	#interacting_timeout_id;

	/**
	 * @type {number}
	 */
	#resize_timeout_id;

	/**
	 * @type {number}
	 */
	#items_max_count = 1000;

	/**
	 * @type {number}
	 */
	#items_loaded_count = 0;

	onActivate() {
		this.#items_max_count = this.#getItemsMaxCount();
	}

	isUserInteracting() {
		return this.#user_interacting || super.isUserInteracting();
	}

	onResize() {
		if (this.getState() !== WIDGET_STATE_ACTIVE) {
			return;
		}

		clearTimeout(this.#resize_timeout_id);

		const old_items_max_count = this.#items_max_count;
		this.#items_max_count = this.#getItemsMaxCount();

		if (this.#items_max_count > old_items_max_count && this.#items_loaded_count >= old_items_max_count) {
			this._startUpdating();
		}

		this.#resize_timeout_id = setTimeout(() => {
			if (this.#honeycomb !== null) {
				this.#honeycomb.setSize(super._getContentsSize());
			}
		}, 100);
	}

	getUpdateRequestData() {
		return {
			...super.getUpdateRequestData(),
			max_items: this.#items_max_count,
			with_config: this.#honeycomb === null ? 1 : undefined
		};
	}

	setContents(response) {
		if (this.#honeycomb === null) {
			const padding = {
				vertical: CWidgetHoneycomb.ZBX_STYLE_DASHBOARD_WIDGET_PADDING_V,
				horizontal: CWidgetHoneycomb.ZBX_STYLE_DASHBOARD_WIDGET_PADDING_H,
			};

			this.#honeycomb = new CSVGHoneycomb(padding, response.config);
			this._body.prepend(this.#honeycomb.getSVGElement());

			this.#honeycomb.setSize(super._getContentsSize());

			this.#honeycomb.getSVGElement().addEventListener(CSVGHoneycomb.EVENT_CELL_CLICK, e => {
				this.broadcast({
					[CWidgetsData.DATA_TYPE_HOST_ID]: [e.detail.hostid],
					[CWidgetsData.DATA_TYPE_HOST_IDS]: [e.detail.hostid],
					[CWidgetsData.DATA_TYPE_ITEM_ID]: [e.detail.itemid],
					[CWidgetsData.DATA_TYPE_ITEM_IDS]: [e.detail.itemid]
				});
			});

			this.#honeycomb.getSVGElement().addEventListener(CSVGHoneycomb.EVENT_CELL_ENTER, e => {
				clearTimeout(this.#interacting_timeout_id);
				this.#user_interacting = true;
			});

			this.#honeycomb.getSVGElement().addEventListener(CSVGHoneycomb.EVENT_CELL_LEAVE, e => {
				this.#interacting_timeout_id = setTimeout(() => {
					this.#user_interacting = false;
				}, 1000);
			});
		}

		this.#items_loaded_count = response.cells.length;

		this.#honeycomb.setValue({
			cells: response.cells
		});
	}

	onClearContents() {
		if (this.#honeycomb !== null) {
			this.#honeycomb.destroy();
			this.#honeycomb = null;
		}
	}

	onDestroy() {
		this.clearContents();
	}

	onFeedback({type, value, descriptor}) {
		if (type === CWidgetsData.DATA_TYPE_ITEM_ID) {
			return this.#honeycomb.selectCell(value);
		}

		return false;
	}

	getActionsContextMenu({can_copy_widget, can_paste_widget}) {
		const menu = super.getActionsContextMenu({can_copy_widget, can_paste_widget});

		if (this.isEditMode()) {
			return menu;
		}

		let menu_actions = null;

		for (const search_menu_actions of menu) {
			if ('label' in search_menu_actions && search_menu_actions.label === t('Actions')) {
				menu_actions = search_menu_actions;

				break;
			}
		}

		if (menu_actions === null) {
			menu_actions = {
				label: t('Actions'),
				items: []
			};

			menu.unshift(menu_actions);
		}

		menu_actions.items.push({
			label: t('Download image'),
			disabled: this.#honeycomb === null,
			clickCallback: () => {
				downloadSvgImage(this.#honeycomb.getSVGElement(), 'image.png');
			}
		});

		return menu;
	}

	hasPadding() {
		return false;
	}

	#getItemsMaxCount() {
		let {width, height} = super._getContentsSize();

		width -= CWidgetHoneycomb.ZBX_STYLE_DASHBOARD_WIDGET_PADDING_H * 2;
		height -= CWidgetHoneycomb.ZBX_STYLE_DASHBOARD_WIDGET_PADDING_V * 2;

		const {max_rows, max_columns} = CSVGHoneycomb.getContainerMaxParams({width, height});

		return Math.min(this.#items_max_count, max_rows * max_columns);
	}
}
