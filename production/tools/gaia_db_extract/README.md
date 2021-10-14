# Database Extraction Utility
This is a utility used by VS Code funtionality to display a grid of data in the VS Code Terminal window. The command-line parameters dictate the output, which is printed to stdout. Types of output include
  - catalog
  - range of rows

The output will be in JSON format.

## JSON format for catalog.
```
{
    "database": "<dbname>",
    "tables": [
        {
            "table": "<tablename>",
            "fields": [
                "field": {"name": "<fieldname>", "type": "<fieldtype>"},
                ...
            ],
            ...
        }
    ]
}
```
