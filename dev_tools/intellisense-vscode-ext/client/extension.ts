const vscode = require('vscode');
const hover_info = require('./hover_info.json')

function activate(context) {
    // Optimize hover_info.json for search.
    const hover_info_map = new Map(Object.entries(Object.assign({}, ...Object.values(hover_info))))

    console.log('"Gaia Platform Intellisense" is now active!');

    vscode.languages.registerHoverProvider('ruleset', {
        provideHover(document, position, token) {
            const range = document.getWordRangeAtPosition(position);
            const word = document.getText(range);

            // Constructs each hover based on its value in hover_info.json.
            if (hover_info_map.has(word)) {
                return new vscode.Hover({
                    language: "ruleset",
                    value: hover_info_map.get(word)
                });
            }
        }
    });
}

function deactivate() { }

module.exports = {
    activate,
    deactivate
}
