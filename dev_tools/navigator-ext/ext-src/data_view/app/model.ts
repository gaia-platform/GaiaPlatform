export interface ITableView {
  db_name: string;
  table_name : string;
  columns: IColumn[];
  rows : any;
}

export interface IColumn {
  key : string;
  name : string;
}

export interface ICommand {
  action: CommandAction;
  link: ILink;
}

// The link defines the parent row id
// and link name.
export interface ILink {
  db_name : string;
  table_name : string;
  link_name : string;
  link_row : number;
}

// Actions that are commanded from the webview to the
// extension.
export enum CommandAction {
  ShowRelatedRecords
}
