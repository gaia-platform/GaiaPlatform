// import { CppToolsApi, Version, CustomConfigurationProvider, getCppToolsApi } from 'vscode-cpptools';
const vscode = require('vscode');
const hover_info = require('./hover_info.json')

//hover_info[property][word]

function activate(context) {

    console.log('"Gaia Platform Intellisense" is now active!');

    vscode.languages.registerHoverProvider('ruleset', {
        provideHover(document, position, token) {
            const range = document.getWordRangeAtPosition(position);
            const word = document.getText(range);
            //function that constructs each hover based on its value in hover_info.json
            for (const property in hover_info){
                if (hover_info[property].hasOwnProperty(word)) {
                    console.log('Working!')
                    return new vscode.Hover({
                        language: "ruleset",
                        value: hover_info[property][word]
                    });
                }
            }
        }
    });
}

function deactivate() { }

module.exports = {
    activate,
    deactivate
}