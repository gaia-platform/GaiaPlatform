import * as vscode from 'vscode';
import * as child_process from 'child_process';
import { ITableView } from './data_view/app/model';

// Interacts with the gaia_db_extract tool to get metadata from
// the catalog as well as row data for each table.  Allow multiple
// class instances the GaiaDataProvider to manage a static instance
// of the catalog
export class GaiaDataProvider {
  constructor() {
  }

  static getDatabases(refresh? : boolean) : any {
    this.populateCatalog(refresh);
    return this.catalog ? this.catalog.databases : undefined;
  }

  static getTables(db_id : number, refresh? : boolean): void {
    this.populateCatalog(refresh);
    return this.catalog ? this.catalog.databases[db_id].tables : undefined;
  }

  static getFields(db_id : number, table_id: number, refresh? : boolean): void {
    this.populateCatalog(refresh);
    return this.catalog ? this.catalog.databases[db_id].tables[table_id].fields : undefined;
  }

  // Never cache table data.
  static getTableData(db_name : string, table_name : string, fields? : any) {

    var child = child_process.spawnSync(this.extract_cmd,
      [`--database=${db_name}`, `--table=${table_name}`]);
    var resultText = child.stderr.toString().trim();
    if (resultText) {
        vscode.window.showErrorMessage(resultText);
        return undefined;
    }

    // We've got the data but we need to get the catalog metadata for the
    // column information if it is not passed in.
    if (!fields) {
      fields = this.findFields(db_name, table_name);
      if (fields === undefined) {
          vscode.window.showInformationMessage(`Table ${table_name} has no columns.`)
          return undefined;
      }
    }

    // Add the row_id to the columns list for data.
    const data = JSON.parse(child.stdout.toString());
    var cols = [{key: 'row_id', name : 'row_id'}];
    for (var i = 0; i < fields.length; i++) {
      cols.push({ key : fields[i], name : fields[i]});
    }

    let tableData : ITableView = {
        db_name : db_name,
        table_name : table_name,
        columns : cols,
        rows : data.rows || [],
    };

    return tableData;
  }

  private static findFields(db_name : string, table_name:string) {
    this.populateCatalog();
    if (!this.catalog) {
      return undefined;
    }

    var databases = this.catalog.databases;
    if (!databases) {
      return undefined;
    }

    for (var db_index = 0; db_index < databases.length; db_index++) {
      if (databases[db_index].name == db_name) {
        var tables = databases[db_index].tables;
        if (!tables) {
          return undefined;
        }
        for (var table_index = 0; table_index < tables.length; table_index++) {
          if (tables[table_index].name == table_name) {
            var catalog_fields = tables[table_index].fields;
            if (!catalog_fields) {
              return undefined;
            }
            var fields = [];
            for (let catalog_field of catalog_fields) {
                fields.push(catalog_field.name);
            }
            return fields;
          }
        }
      }
    }

    return undefined;
  }

  private static populateCatalog(refresh? : boolean ) {
    refresh = refresh || false;
    if (this.catalog && !refresh) {
      return;
    }

    var child = child_process.spawnSync(this.extract_cmd);
    var resultText = child.stderr.toString().trim();
    if (resultText) {
      vscode.window.showErrorMessage(resultText);
      return;
    }

    this.catalog = JSON.parse(child.stdout.toString());
  }

  static catalog:any = undefined;
  static readonly extract_cmd:string = '/opt/gaia/bin/gaia_db_extract';
}

