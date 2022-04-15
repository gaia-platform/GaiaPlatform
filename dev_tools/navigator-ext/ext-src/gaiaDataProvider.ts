import * as vscode from 'vscode';
import * as child_process from 'child_process';
import { ILink, ITableView } from './data_view/app/model';

// Interacts with the gaia_db_extract tool to get metadata from
// the catalog as well as row data for each table.  Allow multiple
// class instances the GaiaDataProvider to manage a static instance
// of the catalog
export class GaiaDataProvider {
  constructor() {
  }

  static getDatabases(refresh? : boolean) : any {
    this.populateCatalog(refresh);
    return this.exists() ? this.catalog.databases : undefined;
  }

  static getTables(db_id : number, refresh? : boolean): void {
    this.populateCatalog(refresh);
    return this.exists(db_id) ? this.catalog.databases[db_id].tables : undefined;
  }

  static getFields(db_id : number, table_id: number, refresh? : boolean): void {
    this.populateCatalog(refresh);
    return this.exists(db_id, table_id) ? this.catalog.databases[db_id].tables[table_id].fields : undefined;
  }

  // Never cache table data.
  static getTableData(link : ILink) {
    // We've got the data but we need to get the catalog metadata for the
    // column information.
    var table = this.findTable(link.db_name, link.table_name);
    if (!table) {
      vscode.window.showInformationMessage(`Table '${link.table_name}' was not found.`);
      return undefined;
    }

    // If this link is requesting related rows then the link_name and link_row fields
    // will be filled in.  In this case we want to request rows from the
    // related table.
    var db_extract_arguments = [`--database=${link.db_name}`, `--table=${link.table_name}`];
    if (link.link_name != undefined && link.link_row != undefined) {
      table = this.findRelatedTable(table, link);
      if (!table) {
        vscode.window.showInformationMessage(`Could not find table for link '${link.link_name}'.`);
        return undefined;
      }
      db_extract_arguments.push(`--link-name=${link.link_name}`);
      db_extract_arguments.push(`--link-row=${link.link_row}`);
    }

    // Fetch the data by running the extract tool
    var child = child_process.spawnSync(this.extract_cmd, db_extract_arguments);
    var resultText = child.stderr.toString().trim();
    if (resultText) {
        vscode.window.showErrorMessage(resultText);
        return undefined;
    }

    const fields = this.findFields(table);
    if (!fields) {
      vscode.window.showInformationMessage(`Table ${table.name} has no columns.`)
      return undefined;
    }

    const relationships = this.getRelationships(table);

    // Add the row_id to the columns list as a "generated column"
    const data = JSON.parse(child.stdout.toString());
    var cols = [{key: 'row_id', name : this.getGeneratedFieldName('row_id'), is_link : false}];

    // Add any relationships as "generated columns"
    for (var i = 0; i < relationships.length; i++) {
      var relationship = relationships[i];
      cols.push({
        key: relationship.link_name,
        name : this.getGeneratedFieldName(relationship.link_name), is_link : true
      });
    }

    // Add the table's columns now.
    for (var i = 0; i < fields.length; i++) {
      cols.push({ key : fields[i], name : fields[i], is_link : false});
    }

    let tableData : ITableView = {
        db_name : link.db_name,
        table_name : table.name,
        columns : cols,
        rows : data.rows || [],
    };

    return tableData;
  }

  private static getGeneratedFieldName(name : string) {
    return '<' + name + '>';
  }

  // Verify that all references exist in the JSON up to the last optional
  // parameter passed in.
  private static exists(db_id? : number, table_id? : number) {
    if (!this.catalog) {
      return false;
    }

    if (db_id != undefined) {
      var databases = this.catalog.databases;
      if (databases == undefined || databases[db_id] == undefined) {
        return false;
      }
      if (table_id != undefined) {
        var tables = databases[db_id].tables;
        if (tables == undefined || tables[table_id] == undefined) {
          return false;
        }
      }
    }

    return true;
  }

  private static findTable(db_name : string, table_name : string) {
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
             return tables[table_index];
           }
         }
      }
    }

    return undefined;
  }

  private static findRelatedTable(table : any, link : ILink) {
    const relationships = this.getRelationships(table);
    for (let i = 0; i < relationships.length; i++) {
      let relationship = relationships[i];
      if (relationship.link_name == link.link_name) {
        return this.findTable(link.db_name, relationship.table_name)
      }
    }
    return undefined;
  }

  private static findFields(table : any) {
    var catalog_fields = table.fields;
    if (!catalog_fields) {
      return undefined;
    }

    var fields = [];
    for (let catalog_field of catalog_fields) {
        fields.push(catalog_field.name);
    }
    return fields;
  }

  private static getRelationships(table : any) {
    return table.relationships;
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

