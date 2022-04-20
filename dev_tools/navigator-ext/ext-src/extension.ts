'use strict';

import * as vscode from 'vscode';
import { GaiaCatalogProvider } from './databaseExplorer';
import ViewLoader from './data_view/ViewLoader';

export function activate(context: vscode.ExtensionContext) {
    vscode.commands.registerCommand('databases.showRecords', item => {
            ViewLoader.showRecords(context.extensionPath, item, context.extensionUri);
    });
    vscode.commands.registerCommand('databases.showRelatedRecords', link => {
        ViewLoader.showRelatedRecords(context.extensionPath, link, context.extensionUri);
    });

    const gaiaProvider = new GaiaCatalogProvider();
    vscode.window.registerTreeDataProvider('databases', gaiaProvider);
    vscode.commands.registerCommand('databases.refreshEntry', () =>
        gaiaProvider.refresh()
    );

    //vscode.window.createTreeView('databases',
    //    { gaiaCatalogProvider : new GaiaCatalogProvider()
    //});
}
