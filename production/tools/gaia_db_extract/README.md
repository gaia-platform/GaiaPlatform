# Database Extraction Utility
This is a utility used by VS Code functionality to display a grid of data in the VS Code Terminal window. The command-line parameters dictate the output, which is printed to stdout. Types of output include:
  - catalog (all database, table and field values in the catalog)
  - range of rows (rows from a single table, showing all field values)

Note: There is currently no support for arrays.

## JSON Output
The output will be in JSON format.

### JSON format for catalog.
```bash
{
    "databases": [
        {
            "name": "<database_name>",
            "tables": [
                {
                    "name": "<table_name>",
                    "fields": [
                        {
                            "name": "<field_name>",
                            "type": "<field_type>"
                        },
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
### JSON format for a range of rows.
```bash
{
    "database": <database_name>,
    "table": "<table_name>",
    "rows": [
        {
            "row_id": <gaia_id>,
            "field1": "<field1_value>",
            "field2": "<field2_value>",
            ...
        },
        ...
    ]
}
```

## Command-line
```bash
gaia_db_extract [--database=<database_name>] [--table=<table_name>] [--start-after=ID] [--row-limit=N]
```
To obtain the catalog (shown above as JSON document), use the command without any parameters:
```bash
gaia_db_extract
```
To obtain the row values for an entire table, specify the database and table name:
```bash
gaia_db_extract --database=<database_name> --table=<table_name>
```
where `<database_name>` and `<table_name>` can be found in the catalog.

To limit the number of rows extracted, use the `--row-limit` parameter:
```bash
gaia_db_extract --database=<database_name> --table=<table_name> --row-limit=N
```

To obtain the block of rows following an already-fetched block of rows, provide the `gaia_id` of the row before the start of the next block. This `gaia_id` is the last one (highest number)
of the previously fetched block. Since the `gaia_id` of the first row of the next block is unknown, the utility will locate a previously known `gaia_id` and start with the one after that.

For example, the following command-line will produce two `gaia_field` rows, as shown after the command:
```bash
gaia_db_extract --database=catalog --table=gaia_field --row-limit=2
{
    "database": "catalog",
    "rows": [
        {
            "active": 0,
            "deprecated": 0,
            "name": "name",
            "position": 0,
            "repeated_count": 1,
            "row_id": 3,
            "type": 11,
            "unique": 0
        },
        {
            "active": 0,
            "deprecated": 0,
            "name": "name",
            "position": 0,
            "repeated_count": 1,
            "row_id": 5,     // <-- Note, this is the last row_id.
            "type": 11,
            "unique": 0
        }
    ],
    "table": "gaia_field"
}
```

To view the next two `gaia_field` rows, add the -`-start-after` parameter. Use the value `5` because `5` is the `row_id` of the last row:
```bash
gaia_db_extract --database=catalog --table=gaia_field --row-limit=2 --start-after=5
{
    "database": "catalog",
    "rows": [
        {
            "active": 0,
            "deprecated": 0,
            "name": "type",
            "position": 1,
            "repeated_count": 1,
            "row_id": 6,
            "type": 6,
            "unique": 0
        },
        {
            "active": 0,
            "deprecated": 0,
            "name": "is_system",
            "position": 2,
            "repeated_count": 1,
            "row_id": 7,
            "type": 0,
            "unique": 0
        }
    ],
    "table": "gaia_field"
}
```

This progression may continue until there are no remaining rows. In this example, the next value for `--start-after` should be `7`. The utility will print `{}` if there are no more rows to extract.
