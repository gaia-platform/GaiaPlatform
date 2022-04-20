export interface ITableView {
  db_name: string;
  table_name : string;
  columns: IColumn[];
  rows : any;
}

export interface IColumn {
  key : string;
  name : string;
  is_link : boolean;
  is_array : boolean;
}

export interface ICommand {
  action: CommandAction;
  link: ILink;
}

// The link defines the database and table information.
// If this is a request to get rows related to the table
// then a link_name and link_row are provided.
export interface ILink {
  db_name : string;
  table_name : string;
  link_name? : string;
  link_row? : number;
}

// Actions that are commanded from the webview to the
// extension.
export enum CommandAction {
  ShowRelatedRecords
}
