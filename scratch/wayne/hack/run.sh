# Set this in motion in the Docker container.
# Run 'pkill gaia_db_server' to reset the database.
sudo chmod a+w /var/log
until gaia_db_server --disable-persistence >> /var/log/db_server.log 2>&1; do
    echo "`date`: gaia_db_server crashed, exit code $?.  Restarting.." >> /var/log/db_server.log 2>&1
    sleep 1
done
