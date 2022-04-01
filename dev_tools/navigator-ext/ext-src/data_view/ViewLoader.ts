'use strict';

import * as path from 'path';
import * as vscode from 'vscode';
import * as child_process from 'child_process';

import { CatalogItem } from '../databaseExplorer';
import { ITableView } from './app/model';



// Manages react webview panels.
export default class ViewLoader {
    // Track the currently panel.
    // Only allow a single panel to exist at a time.

    // todo (dax): need currentPanel?
    public static currentViews:any = {};

    private static readonly viewType = 'react';

    private readonly _panel: vscode.WebviewPanel;
    private readonly _title: string;
    private readonly _extensionPath: string;
    private _disposables: vscode.Disposable[] = [];

    public static createOrShow(extensionPath: string, item : CatalogItem) {
        const column = vscode.window.activeTextEditor ? vscode.window.activeTextEditor.viewColumn : undefined;
        var title = `${item.db_name}.${item.label}`;

        // If we already have a panel with this title, then show it
        if (ViewLoader.currentViews[title]) {
            ViewLoader.currentViews[title]._panel.reveal(column);
        } else {
            ViewLoader.currentViews[title] = new ViewLoader(
                extensionPath,
                title,
                item,
                column || vscode.ViewColumn.One);
        }
    }

    private constructor(extensionPath: string, title: string, table: CatalogItem, column: vscode.ViewColumn) {
        this._extensionPath = extensionPath;
        this._title = title;

        // Create and show a new webview panel.
        this._panel = vscode.window.createWebviewPanel(ViewLoader.viewType, this._title, column, {
            // Enable javascript in the webview.
            enableScripts: true,

            // And restrict the webview to only loading content from our extension's `media` directory.
            localResourceRoots: [
                vscode.Uri.file(path.join(this._extensionPath, 'dataViewer'))
            ]
        });

        // Set the webview's initial html content.
        this._panel.webview.html = this._getHtmlForWebview(table);

        // Listen for when the panel is disposed.
        // This happens when the user closes the panel or when the panel is closed programatically.
        this._panel.onDidDispose(() => this.dispose(), null, this._disposables);

        // Handle messages from the webview.
        this._panel.webview.onDidReceiveMessage(message => {
            switch (message.command) {
                case 'alert':
                    vscode.window.showErrorMessage(message.text);
                    return;
            }
        }, null, this._disposables);
    }

    public doRefactor() {
        // Send a message to the webview webview.
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

    private _getHtmlForWebview(table : CatalogItem) {
//        const manifest = require(path.join(this._extensionPath, 'build', 'asset-manifest.json'));
//        const mainScript = manifest.files['main.js'];
//        const mainStyle = manifest.files['main.css'];

        // Here 'app' refers to the react application running inside the webview.
        const appPathOnDisk = vscode.Uri.file(path.join(
            this._extensionPath, 'dataViewer', 'dataViewer.js'));

        const appUri = appPathOnDisk.with({scheme: "vscode-resource"});

0       //const stylePathOnDisk = vscode.Uri.file(path.join(this._extensionPath, 'build', mainStyle));
        //const styleUri = stylePathOnDisk.with({ scheme: 'vscode-resource' });



        // Use a nonce to whitelist which scripts can be run
        const nonce = getNonce();
        const tableData = this._loadData(table);
        if (!tableData)
        {
            return`<!DOCTYPE html>
                <html lang="en">
                <head>
                    <meta charset="utf-8">
                    <meta name="viewport" content="width=device-width,initial-scale=1.0,shrink-to-fit=yes">
                    <meta name="theme-color" content="#000000">
                </head>
                <body>
                    <div id="root">
                    <h2> An error occurred retrieving data for table '${table.label}'.</h2>
                    </div>
                </body>
                </html>`;
        }

        const tableJson = JSON.stringify(tableData);

        // todo (dax): not sure all this html below is necessary.
        return `<!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="utf-8">
                <meta name="viewport" content="width=device-width,initial-scale=1.0,shrink-to-fit=no">
                <meta name="theme-color" content="#000000">
                <title>Data View</title>
                <meta http-equiv="Content-Security-Policy"
                    content="default-src 'none';
                    img-src vscode-resource: https:;
                    script-src 'unsafe-eval' 'unsafe-inline' vscode-resource:;
                    style-src vscode-resource: 'unsafe-inline' http: https: data:;">
                <script>
                    window.acquireVsCodeApi = acquireVsCodeApi;
                    window.initialData = ${tableJson};
                </script>
            </head>
            <body>
                <div id="root"></div>
                <script src="${appUri}"></script>
            </body>
            </html>`;

            /**old school
            return `<!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="utf-8">
                <meta name="viewport" content="width=device-width,initial-scale=1.0,shrink-to-fit=no">
                <meta name="theme-color" content="#000000">
                <title>Data View</title>
                <meta http-equiv="Content-Security-Policy"
                    content="default-src 'none';
                    img-src vscode-resource: https:;
                    script-src 'nonce-${nonce}';style-src vscode-resource: vscode-resource:;
                    style-src vscode-resource: 'unsafe-inline;">

                // this worked
                <meta http-equiv="Content-Security-Policy"
                    content="default-src 'none';
                    img-src https:;
                    script-src 'unsafe-eval' 'unsafe-inline' vscode-resource:;
                    style-src vscode-resource: 'unsafe-inline';">

                <script>
                    window.acquireVsCodeApi = acquireVsCodeApi;
                    window.initialData = ${tableJson};
                </script>
            </head>
            <body>
                <div id="root"></div>
                <script nonce="${nonce}" src="${appUri}"></script>
            </body>
            </html>`;
            */

    }

    private _loadData(table: CatalogItem) {
        var child = child_process.spawnSync('/opt/gaia/bin/gaia_db_extract',
            [`--database=${table.db_name}`, `--table=${table.label}`]);

        var resultText = child.stderr.toString().trim();
        if (resultText) {
            vscode.window.showErrorMessage(resultText);
            return undefined;
        }

        if (table.fields === undefined) {
            vscode.window.showInformationMessage(`Table {table.label} has no columns.`)
            return undefined;
        }

        const gaiaJson = JSON.parse(child.stdout.toString());
        const cols = table.fields.map((item : string) => {
            return { key : item, name : item};
        })

        let tableData : ITableView = {
            columns : cols,
            rows : gaiaJson.rows || [],
        };

        return tableData;
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