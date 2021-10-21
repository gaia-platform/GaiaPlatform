# Database Extraction Utility
This is a utility used by VS Code funtionality to display a grid of data in the VS Code Terminal window. The command-line parameters dictate the output, which is printed to stdout. Types of output include
  - catalog
  - range of rows

## JSON Output
The output will be in JSON format.

### JSON format for catalog.
```
{
    "databases": [
        {
            "name": "<dbname>",
            "tables": [
                {
                    "name": "<tablename>",
                    "fields": [
                        {
                            "name": "<fieldname>",
                            "type": "<fieldtype>"
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
```
{
    "database": <databasename>,
    "table": "<tablename>",
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
```
gaia_db_extract [--database=<dbname>] [--table=<tableneme>] [--start-after=ID] [--row-limit=N]
```
To obtain the catalog (shown above), use the command without any parameters:
```
gaia_db_extract
```
To obtain the row values for an entire table, specify the database and table name:
```
gaia_db_extract --database=<dbname> --table=<tablename>
```
where `<dbname>` and `<tablename>` can be found in the catalog.

To limit the number of rows extracted, use the `--row-limit` parameter:
```
gaia_db_extract --database=<dbname> --table=<tablename> --row-limit=N
```

To obtain the block of rows following an already-fetched block of rows, provide the `gaia_id`
of the row before the start of the next block. This `gaia_id` is the last one (hightest number)
of the previously fetched block. Since the `gaia_id` of the first row of the next block is unknown,
the utility will locate a previously known `gaia_id` and start with the one after that.

For example, the following command-line will produce two `gaia_field` rows, as shown after
the command:
```
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

To view the next two `gaia_field` rows, add the -`-start-after` parameter. Use the value `5`
since `5` is the `row_id` of the last row:
```
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

This progression may continue until there are no remaining rows. In this example, the next value
for `--start-after` should be `7`. The utility will print `null` if there are no
more rows to extract.
