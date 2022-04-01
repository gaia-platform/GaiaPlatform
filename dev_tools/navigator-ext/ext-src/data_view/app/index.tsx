import * as React from 'react';
import * as ReactDOM from 'react-dom';
import './index.css';
import DataView from "./dataview";
import {ITableView} from "./model";

declare global {
    interface Window {
        acquireVsCodeApi():any;
        initialData: ITableView;
    }
}

const vscode = window.acquireVsCodeApi();

ReactDOM.render(
    <DataView vscode={vscode} initialData={window.initialData} />,
    document.getElementById('root') as HTMLElement
);
