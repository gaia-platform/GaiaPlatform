'use strict';

import * as vscode from 'vscode';
import { GaiaCatalogProvider } from './databaseExplorer';
import ViewLoader from './data_view/ViewLoader';

export function activate(context: vscode.ExtensionContext) {
    vscode.commands.registerCommand('databases.showRecords', item => {
            ViewLoader.showRecords(context.extensionUri, item);
    });
    vscode.commands.registerCommand('databases.showRelatedRecords', link => {
        ViewLoader.showRelatedRecords(context.extensionUri, link);
    });

    const gaiaProvider = new GaiaCatalogProvider();
    vscode.window.registerTreeDataProvider('databases', gaiaProvider);
    vscode.commands.registerCommand('databases.refreshEntry', () =>
        gaiaProvider.refresh()
    );

    context.subscriptions.push(vscode.window.onDidChangeActiveColorTheme( () =>
        ViewLoader.applyTheme()
    ));
}
