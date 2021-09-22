<<<<<<< HEAD
<<<<<<< HEAD
import { hover_info } from './hover_info';
import { 
    ExtensionContext,
    languages,
    Hover,
    MarkdownString
} from 'vscode';

function activate(context: ExtensionContext) {

    // Creates map out of hover_info keys/values.
    const hover_info_map = new Map(Object.entries(hover_info))

    // Creates provider to give the extension hover functionality.
    languages.registerHoverProvider('ruleset', {
        provideHover(document, position, token) {

            // Finds the line that the user's cursor is hovering over.
            const range = document.getWordRangeAtPosition(position);

            // Gets the word that the cursor is hovering over given the line.
            const word: string = document.getText(range);

            // Constructs each hover based on its value in hover_info_map.
            if (hover_info_map.has(word)) {

                // Creates markdown string to be used in hover.
                let markdown_string = new MarkdownString(`${hover_info_map.get(word)}`)

                // Indicates that the markdown string is from a trusted source.
                markdown_string.isTrusted = true;
=======
// const vscode = require('vscode');
// const hover_info = require('..hover_info.json');
=======
>>>>>>> d987c1231 (removed extra comments)
import { hover_info } from './hover_info';
import { 
    workspace,
    ExtensionContext,
    languages,
    Hover,
	MarkdownString
} from 'vscode';

import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

function activate(context: ExtensionContext) {

	//creates map out of hover_info keys/values.
	const hover_info_map = new Map(Object.entries(hover_info))

	//creates provider to give the extension hover functionality.
    languages.registerHoverProvider('ruleset', {
        provideHover(document, position, token) {

			//Finds the line that the user's cursor is hovering over.
            const range = document.getWordRangeAtPosition(position);

			//Gets the word that the cursor is hovering over given the line.
            const word: string = document.getText(range);

            //Constructs each hover based on its value in hover_info_map.
            if (hover_info_map.has(word)) {

				//Creates markdown string to be used in hover.
				let markdown_string = new MarkdownString(`${hover_info_map.get(word)}`)

				//Indicates that the markdown string is from a trusted source.
				markdown_string.isTrusted = true;
>>>>>>> f837722ec (markdown strings for hovers)
                return new Hover(markdown_string);
            }
        }
    });
}

function deactivate() { }

module.exports = {
    activate,
    deactivate
}
