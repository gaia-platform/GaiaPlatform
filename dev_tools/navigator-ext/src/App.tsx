//import * as vscode from 'vscode';
import * as React from 'react';
import './App.css';


// another candidate is gridjs

//import { Grid } from 'gridjs-react';
//import { Grid } from "gridjs";
//import "gridjs/dist/theme/mermaid.css";

import DataGrid from 'react-data-grid';
import * as child_process  from 'child_process'

const columns = [
    { key: 'id', name: 'ID' },
    { key: 'title', name: 'Title' }
  ];

  const rows = [
    { id: 0, title: 'Example' },
    { id: 1, title: 'Demo' }
  ];

function App() {

  var child = child_process.spawnSync('/opt/gaia/bin/gaia_db_extract --database=catalog --table=gaia_field');
  var resultText = child.stderr.toString().trim();
  if (resultText) {
//    vscode.window.showErrorMessage(resultText);
    return;
  }


  const gaiaJson = JSON.parse(child.stdout.toString());
  var rows = gaiaJson.rows;

  return <DataGrid className='rdg-dark' columns={columns} rows = {rows}/>;
}

export default App;
