import * as React from 'react';
import DataGrid from 'react-data-grid';
import {CommandAction, IColumn, ILink, ICommand} from "./model";
import ShowRecordsDarkIcon from '../../../resources/dark/boolean.svg';
import ShowRecordsLightIcon from '../../../resources/light/boolean.svg';
import {VSCodeButton} from "@vscode/webview-ui-toolkit/react"

function DataView(props : any) {
    let initialData = props.initialData;
    let vscode = props.vscode;

    var theme = document.body.className;
    var gridTheme = 'rdg-light';
    var RecordsIcon = ShowRecordsLightIcon;
    if (theme === 'vscode-dark' || theme == 'vscode-high-contrast') {
      gridTheme = 'rdg-dark';
      RecordsIcon = ShowRecordsDarkIcon;
    }

    for (var i = 0; i < initialData.columns.length; i++) {
      let col = initialData.columns[i];
      if (col.is_link) {
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
      }
    }

   return <DataGrid className={gridTheme} style={{height:window.innerHeight}} columns={initialData.columns} rows={initialData.rows} />
}

export default DataView;
