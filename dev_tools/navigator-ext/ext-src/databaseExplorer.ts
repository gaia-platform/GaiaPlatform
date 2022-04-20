import * as vscode from 'vscode';
import * as path from 'path';
import { GaiaDataProvider } from './gaiaDataProvider';

export class GaiaCatalogProvider implements vscode.TreeDataProvider<CatalogItem> {
  private _onDidChangeTreeData: vscode.EventEmitter<CatalogItem | undefined | null | void> = new vscode.EventEmitter<CatalogItem | undefined | null | void>();
  readonly onDidChangeTreeData: vscode.Event<CatalogItem | undefined | null | void> = this._onDidChangeTreeData.event;

  constructor() {
  }

  refresh(): void {
    GaiaDataProvider.clear();
    this._onDidChangeTreeData.fire();
  }

  getTreeItem(element: CatalogItem): vscode.TreeItem {
    return element;
  }

  getChildren(element?: CatalogItem): Thenable<CatalogItem[]> {
    if (element) {
        return Promise.resolve(this.getCatalogItems(element));
    }
    else {
        return Promise.resolve(this.getCatalogItems())
    }
  }

  private getCatalogItem(
    name : string,
    db_id : number,
    db_name : string,
    table_id? : number,
    table_type? : number,
    fields? : string[],
    column_id? : number,
    position? : number,
    type? : string,
    is_array? : boolean) {
      return new CatalogItem(
        vscode.TreeItemCollapsibleState.Collapsed,
        name, db_id, db_name, table_id, table_type, fields, column_id, position, type, is_array);
  }

  private getCatalogItems(element?: CatalogItem): CatalogItem[] {
    // We only read databases -> tables -> columns.
    // If this is a column, there there is no more data to read.
    if (element && element.column_id != undefined) {
      return [];
    }

    var items = GaiaDataProvider.getDatabases();
    if (!items) {
      return [];
    }

    // Return the databases if we were not passed a parent catalog item.
    if (!element) {
      return Object.keys(items).map((catalogItem, index) =>
        this.getCatalogItem(items[catalogItem].name, index, items[catalogItem].name)
      )
    }

    // Otherwise, this is a table element and we iterate over the table's fields.
    if (element.table_id != undefined) {
      items = GaiaDataProvider.getFields(element.db_id, element.table_id);
      return Object.keys(items).map((catalogItem, index) =>
        this.getCatalogItem(items[catalogItem].name,
          element.db_id, element.db_name, element.table_id, element.table_type, element.fields, index,
          items[catalogItem].position, items[catalogItem].type, items[catalogItem].repeated_count == 0))
    }


    // If a table_id is not defined then iterate over the database's tables.
    items = GaiaDataProvider.getTables(element.db_id);
    if (!items) {
      return [];
    }
    return Object.keys(items).map((catalogItem, index) => {
      var fields = [];
          var catalog_fields = items[catalogItem].fields;
          if (catalog_fields) {
            for (let catalog_field of catalog_fields) {
              fields.push(catalog_field.name);
            }
          }
          return this.getCatalogItem(items[catalogItem].name, element.db_id, element.db_name, index, items[catalogItem].type, fields);
    });
  }
}

export class CatalogItem extends vscode.TreeItem {
  constructor(
    public readonly collapsibleState: vscode.TreeItemCollapsibleState,
    public readonly label: string,
    public readonly db_id : number,
    public readonly db_name : string,
    public readonly table_id? : number,
    public readonly table_type? : number,
    public readonly fields? : string[],
    public readonly column_id? : number,
    public readonly position? : number,
    public readonly type? : string,
    public readonly is_array? : boolean
  )
  {
    super(label, collapsibleState);
    if (this.label == "")    {
      this.label = "<default>";
    }
    var brackets = is_array ? "[]" : "";
    if (column_id != undefined)    {
      this.tooltip = `column type: ${this.type}${brackets}, position: ${this.position}`;
      this.contextValue = 'column';
      this.iconPath = {
        light: path.join(__filename, '..', '..', 'resources', 'light', 'column.svg'),
        dark: path.join(__filename, '..', '..', 'resources', 'dark', 'column.svg')
      };

    } else if (this.table_id != undefined)    {
      this.tooltip = `table type: ${this.table_type}`
        this.iconPath = {
          light: path.join(__filename, '..', '..', 'resources', 'light', 'table.svg'),
          dark: path.join(__filename, '..', '..', 'resources', 'dark', 'table.svg')
        };
      this.contextValue = 'table';
    } else {
      this.contextValue = 'database';
    }
  }

  iconPath = {
    light: path.join(__filename, '..', '..', 'resources', 'light', 'catalog.svg'),
    dark: path.join(__filename, '..', '..', 'resources', 'dark', 'catalog.svg')
  };

  contextValue = 'database';
}

