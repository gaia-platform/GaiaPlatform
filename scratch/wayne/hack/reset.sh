# Clear the database and cause the app to get completely rebuild.
pkill gaia_se_server
touch hack-comments.ddl
cmake .
