'use strict';

import * as vscode from 'vscode';
import { GaiaCatalogProvider } from './databaseExplorer';
import ViewLoader from './data_view/ViewLoader';

export function activate(context: vscode.ExtensionContext) {

//    context.subscriptions.push(vscode.commands.registerCommand('database-webview.start', () => {
//        ReactPanel.createOrShow(context.extensionPath);
//    }));

    vscode.commands.registerCommand('databases.showRecords', item => {
            ViewLoader.showRecords(context.extensionPath, item);
    });

    vscode.commands.registerCommand('databases.showRelatedRecords', link => {
        ViewLoader.showRelatedRecords(context.extensionPath, link);
    });

    vscode.window.registerTreeDataProvider('databases', new GaiaCatalogProvider());

    //vscode.window.createTreeView('databases',
    //    { gaiaCatalogProvider : new GaiaCatalogProvider()
    //});
}
