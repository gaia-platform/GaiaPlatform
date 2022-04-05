import * as React from 'react';
//import './dataview.css';
import DataGrid from 'react-data-grid';
import {CommandAction, IColumn, ILink, ICommand} from "./model";
//import Button from 'react-bootstrap/Button';
//import 'bootstrap/dist/css/bootstrap.min.css';


function DataView(props : any) {
    let initialData = props.initialData;
    let vscode = props.vscode;

    // The webview can communicate back to the VS Code extension
    // by posting a message.
    const LinkFormatter = ({row}) => {
      return <button onClick={() => {
        let command : ICommand = {
          action: CommandAction.ShowRelatedRecords,
          link: {
            db_name : initialData.db_name,
            // todo (dax): remove hard-coded values below
            //table_name : initialData.table_name,
            table_name : 'actuator',
            link_name : "actuators",
            link_row : row.row_id
          }
        };
        vscode.postMessage(command);
        }}>
        {row.name}
      </button>
    }

    // Add a formatter to the name column
    // todo (dax): this needs to be a formatter for link columns
    // that are added.
    for (var i = 0; i < initialData.columns.length; i++)
    {
      var col = initialData.columns[i];
      if (col.key == 'name') {
        col['formatter'] = LinkFormatter;
        col['getRowMetaData'] = (row) => row;
      }
    }

    /* todo (dax): manage state here

    let oldState = props.vscode.getState();
    if (oldState) {
    }
    else {
    }
    */

   return <DataGrid className='rdg-dark' columns={initialData.columns} rows={initialData.rows} />
}

export default DataView;
