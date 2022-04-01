'use strict';

import * as vscode from 'vscode';
import { GaiaCatalogProvider } from './databaseExplorer';
import ViewLoader from './data_view/ViewLoader';

export function activate(context: vscode.ExtensionContext) {

//    context.subscriptions.push(vscode.commands.registerCommand('database-webview.start', () => {
//        ReactPanel.createOrShow(context.extensionPath);
//    }));

    vscode.commands.registerCommand('databases.displayRecords', item => {
            ViewLoader.createOrShow(context.extensionPath, item);
    });

    vscode.window.registerTreeDataProvider('databases', new GaiaCatalogProvider());

    //vscode.window.createTreeView('databases',
    //    { gaiaCatalogProvider : new GaiaCatalogProvider()
    //});
}
