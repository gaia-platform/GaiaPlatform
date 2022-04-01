export interface ITableView {
  columns: IColumn[]
  rows : any
}

export interface IColumn {
  key : string,
  name : string
}