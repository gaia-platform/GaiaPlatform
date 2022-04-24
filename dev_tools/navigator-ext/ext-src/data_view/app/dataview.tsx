import * as React from 'react';
import DataGrid from 'react-data-grid';
import {CommandAction, IColumn, ILink, ICommand} from "./model";
import ShowRecordsDarkIcon from '../../../resources/dark/boolean.svg';
import ShowRecordsLightIcon from '../../../resources/light/boolean.svg';
import {VSCodeButton} from "@vscode/webview-ui-toolkit/react"

function getArrayString( arrayData : any ) {
  if (!arrayData || arrayData.length == 0) {
    return '';
  }

  var arrayString = '[';
  for (let i = 0; i < arrayData.length; i++) {
    arrayString += arrayData[i];
    if (i < arrayData.length - 1) {
      arrayString += ', '
    }
  }
  arrayString += ']'
  return arrayString;
}

function getAppearance() {
  var theme = document.body.className;
  var gridTheme = 'rdg-light';
  var recordsIcon = ShowRecordsLightIcon;
  // Unfortunately, we don't get the light or dark "flavor"
  // of high contrast.
  if (theme === 'vscode-dark' || theme == 'vscode-high-contrast') {
    gridTheme = 'rdg-dark';
    recordsIcon = ShowRecordsDarkIcon;
  }

  return { gridClass: gridTheme, RecordsIcon: recordsIcon};
}

function DataView(props : any) {
    let initialData = props.initialData;
    let vscode = props.vscode;

    const { gridClass, RecordsIcon } = getAppearance();

    for (var i = 0; i < initialData.columns.length; i++) {
      let col = initialData.columns[i];
      if (col.is_link) {
        // Note that the row_id is returned in the data but we don't
        // expose it as a column.
        col['formatter'] = ({row}) => {
          return <VSCodeButton appearance="icon" onClick={() => {
            let command : ICommand = {
              action: CommandAction.ShowRelatedRecords,
              link: {
                db_name : initialData.db_name,
                table_name : initialData.table_name,
                link_name : col.key,
                link_row : row.row_id
              }
            };
            vscode.postMessage(command);
          }} >
          <RecordsIcon/>
          </VSCodeButton>
        }
      } else if (col.is_array) {
        col['formatter'] = ({row}) => {
          return getArrayString(row[col.key]);
        }
      }
    }

   return <DataGrid className={gridClass} style={{height:window.innerHeight}} columns={initialData.columns} rows={initialData.rows} />
}

export default DataView;
