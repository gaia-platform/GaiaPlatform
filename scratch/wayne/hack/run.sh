# Set this in motion in the Docker container.
# Run 'pkill gaia_se_server' to reset the database.
sudo chmod a+w /var/log
until gaia_se_server --disable-persistence >> /var/log/se_server.log 2>&1; do
    echo "`date`: gaia_se_server crashed, exit code $?.  Restarting.." >> /var/log/se_server.log 2>&1
    sleep 1
done
