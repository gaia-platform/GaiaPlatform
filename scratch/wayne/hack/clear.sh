# Empty the database and reload the catalog. Clears the added, non-catalog data.
pkill gaia_db_server
sleep 2
gaiac -g hack.ddl
