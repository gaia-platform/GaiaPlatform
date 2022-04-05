import * as React from 'react';
//import './dataview.css';
import DataGrid from 'react-data-grid';
import {CommandAction, IColumn, ILink, ICommand} from "./model";
import Button from 'react-bootstrap/Button';
//import 'bootstrap/dist/css/bootstrap.min.css';


function DataView(props : any) {
    let initialData = props.initialData;
    let vscode = props.vscode;

    for (var i = 0; i < initialData.columns.length; i++) {
      let col = initialData.columns[i];
      if (col.is_link) {
        col['formatter'] = ({row}) => {
          return <Button onClick={() => {
            var name = col.key;
            let command : ICommand = {
              action: CommandAction.ShowRelatedRecords,
              link: {
                db_name : initialData.db_name,
                table_name : initialData.table_name,
                link_name : col.name,
                link_row : row.row_id
              }
            };
            vscode.postMessage(command);
          }}>
            ...
          </Button>
        }
      }
    }

   return <DataGrid className='rdg-dark' columns={initialData.columns} rows={initialData.rows} />
}

export default DataView;
