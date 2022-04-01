import * as React from 'react';
//import './dataview.css';
import DataGrid from 'react-data-grid';

const columns = [
    { key: 'id', name: 'ID' },
    { key: 'title', name: 'Title' }
  ];

const rows = [
    { id: 0, title: 'Example' },
    { id: 1, title: 'Demoof' }
  ];

function DataView(props : any) {
    let initialData = props.initialData;


    /* todo (dax): manage state here

    let oldState = props.vscode.getState();
    if (oldState) {
    }
    else {
    }
    */

   return <DataGrid className='rdg-dark' columns={initialData.columns} rows={initialData.rows} />
//  return <DataGrid className='rdg-dark' columns={columns} rows={rows} />

  /*
  return (
    <React.Fragment>
      <h1>Hello, Fucker!</h1>
    </React.Fragment>);
  */
}

export default DataView;
