# Set this in motion in the Docker container.
# Run 'pkill gaia_se_server' to reset the database.
sudo chmod a+w /var/log
rm -rf /tmp/gaia_db/*
until gaia_se_server >> /var/log/se_server.log 2>&1; do
    echo "`date`: gaia_se_server crashed, exit code $?.  Restarting.." >> /var/log/se_server.log 2>&1
    echo Removing database files.
    rm -rf /tmp/gaia_db/*
    sleep 1
done
