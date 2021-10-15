# Database Extraction Utility
This is a utility used by VS Code funtionality to display a grid of data in the VS Code Terminal window. The command-line parameters dictate the output, which is printed to stdout. Types of output include
  - catalog
  - range of rows

The output will be in JSON format.

## JSON format for catalog.
```
{
    "databases":
    [
        {
            "database": "<dbname>",
            "tables":
            [
                {
                    "table": "<tablename>",
                    "fields":
                    [
                        "field": {"name": "<fieldname>", "type": "<fieldtype>"},
                        ...
                    ]
                },
                ...
            ]
        },
        ...
    ]
}
```
## JSON format for a range of rows.
```
{
    "table": "<tablename>",
    "rows":
    [
        "row_id": <gaia_id>,
        "row": {
            "field1": "<field1_value>",
            "field2": "<field2_value>",
            ...
        },
        ...
    ]
}
```