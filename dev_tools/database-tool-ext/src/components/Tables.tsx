import * as React from 'react';
import './components.css'

// Tables function to be exported for use in MainContainer.
export default function Tables() {
    // Hook to set initial state as placeholder names until database info is accessed.
    const [table_names, useTable_names] = React.useState(
        [
            'Table Name 1',
            'Table Name 2',
            'Table Name 3',
            "Table Name 4",
            'Table Name 5',
            'Table Name 6'
        ]
    )

    // Returns the initial table webview that contains all of the tables in the current database.
    return (
        <table id="tables">
            <thead>
                <tr>
                    <th>Tables</th>
                </tr>
            </thead>
            <tbody>
                {table_names.map(tname => (
                    // Each row will represent a table in the database.
                    <tr>
                        <td>{tname}</td>
                    </tr>
                ))}
            </tbody>
        </table>
    )
}