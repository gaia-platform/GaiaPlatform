# Empty the database and reload the catalog. Clears the added, non-catalog data.
pkill gaia_se_server
sleep 2
gaiac -g hack.ddl
