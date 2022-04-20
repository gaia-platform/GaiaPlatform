'use strict';

import * as path from 'path';
import * as vscode from 'vscode';
import * as child_process from 'child_process';

import { CatalogItem } from '../databaseExplorer';
import { ICommand, CommandAction, ILink } from './app/model';
import { GaiaDataProvider } from '../gaiaDataProvider';
import { getUri } from "./getUri";

// Manages react webview panels.
export default class ViewLoader {
    // Track the currently panel.
    // Only allow a single panel to exist at a time.

    // todo (dax): need currentPanel?
    public static currentViews:any = {};

    private static readonly viewType = 'react';

    private readonly _panel: vscode.WebviewPanel;
    private readonly _title: string;
    private _disposables: vscode.Disposable[] = [];

    // Shows records from a table.
    public static showRecords(extensionUri: vscode.Uri, item : CatalogItem) {
        const column = vscode.window.activeTextEditor ? vscode.window.activeTextEditor.viewColumn : undefined;
        var title = `${item.db_name}.${item.label}`;

        // If we already have a panel with this title, then show it
        if (ViewLoader.currentViews[title]) {
            ViewLoader.currentViews[title]._panel.reveal(column);
        } else {
            ViewLoader.currentViews[title] = new ViewLoader(
                title,
                column || vscode.ViewColumn.One,
                {
                    db_name : item.db_name,
                    table_name : item.label,
                    link_name : undefined,
                    link_row : undefined
                },
                extensionUri
            );
        }
    }

    // Shows related records to a table.
    public static showRelatedRecords(extensionUri: vscode.Uri, link : ILink) {
        const column = vscode.window.activeTextEditor ? vscode.window.activeTextEditor.viewColumn : undefined;

        // See if we can find the metadata for this table via the catalog.  If not,
        // we'll have to look it up.
        // todo (dax): enable/maintain a for catalog items so that we can go directly to a table either
        // via the explorer or by getting a link from a table.

        var title = `${link.table_name}.${link.link_name}`;

        // todo (dax): you can see that you would have to change the function prototypes "all the way down"
        // if you don't pass CatalogItems.  Need to figure out how to pull together ILink and CatalogItem

        // For now, always create a new view since the set of records
        // for the same title may be diffrent depending on the row id of the
        // "parent" record
        ViewLoader.currentViews[title] = new ViewLoader(
            title,
            column || vscode.ViewColumn.One,
            link,
            extensionUri
            );
    }

    private constructor(title: string, column: vscode.ViewColumn, link : ILink, extensionUri : vscode.Uri) {
        this._title = title;

        // Create and show a new webview panel.
        this._panel = vscode.window.createWebviewPanel(ViewLoader.viewType, this._title, column, {
            // Enable javascript in the webview.
            enableScripts: true,

            // And restrict the webview to only loading content from our extension's `media` directory.
            localResourceRoots: [
                extensionUri
            ]
        });

        // Set the webview's initial html content.
        this._panel.webview.html = this._getHtmlForWebview(link, this._panel.webview, extensionUri);

        // Listen for when the panel is disposed.
        // This happens when the user closes the panel or when the panel is closed programatically.
        this._panel.onDidDispose(() => this.dispose(), null, this._disposables);

        // Handle messages from the webview.
        this._panel.webview.onDidReceiveMessage((command : ICommand)  => {
            switch (command.action) {
                case CommandAction.ShowRelatedRecords:
                    vscode.commands.executeCommand('databases.showRelatedRecords', command.link);
                    return;
            }
        }, null, this._disposables);
    }

    public doRefactor() {
        // Send a message to the webview.
        // You can send any JSON serializable data.
        this._panel.webview.postMessage({ command: 'refactor' });
    }

    public dispose() {
        delete ViewLoader.currentViews[this._title];

        // Clean up our resources
        this._panel.dispose();

        while (this._disposables.length) {
            const x = this._disposables.pop();
            if (x) {
                x.dispose();
            }
        }
    }

    private _getHtmlForNoTable(tableName : string) {
        return `<!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="utf-8">
            <meta name="viewport" content="width=device-width,initial-scale=1.0,shrink-to-fit=yes">
        </head>
        <body>
            <div id="root">
            <h2> An error occurred retrieving data for table '${tableName}'.</h2>
            </div>
        </body>
        </html>`;
    }

    private _getHtmlForWebview(link : ILink, webview: vscode.Webview, extensionUri: vscode.Uri) {
        // Here 'app' refers to the react application running inside the webview.
        const scriptPathOnDisk = vscode.Uri.joinPath(extensionUri, 'dataViewer', 'dataViewer.js');
        const scriptUri = webview.asWebviewUri(scriptPathOnDisk);
        const toolkitUri = getUri(webview, extensionUri, [
            "node_modules",
            "@vscode",
            "webview-ui-toolkit",
            "dist",
            "toolkit.js", // A toolkit.min.js file is also available
        ]);

        // Use a nonce to whitelist which scripts can be run
        const nonce = getNonce();

        const tableData = GaiaDataProvider.getTableData(link);
        if (!tableData)
        {
            return this._getHtmlForNoTable(link.table_name);
        }

        const tableJson = JSON.stringify(tableData);

        return `<!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="utf-8">
                <meta name="viewport" content="width=device-width,initial-scale=1.0,shrink-to-fit=no">
                <meta name="theme-color" content="#000000">
                <title>Data View</title>
                <meta http-equiv="Content-Security-Policy"
                    content="default-src 'none';
                    script-src 'nonce-${nonce}';
                    style-src 'unsafe-inline' ${webview.cspSource};">
                <script type="module" src="${toolkitUri}"></script>
                <script nonce="${nonce}">
                    window.acquireVsCodeApi = acquireVsCodeApi;
                    window.initialData = ${tableJson};
                </script>
            </head>
            <body>
                <div id="root"></div>
                <script nonce="${nonce}" src="${scriptUri}"></script>
            </body>
            </html>`;
    }
}

function getNonce() {
    let text = "";
    const possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (let i = 0; i < 32; i++) {
        text += possible.charAt(Math.floor(Math.random() * possible.length));
    }
    return text;
}