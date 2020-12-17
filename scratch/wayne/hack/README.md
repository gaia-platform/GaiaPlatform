# To use:
 - Create Docker image
```
make -f Makefile.hack build
```
 - Run Docker image
```
make -f Makefile.hack run
```
 - Start a command-prompt in the container. Password is "password".
```
make -f Makefile.hack ssh
ssh user@localhost -p2222
user@localhost's password: "password"
user@5322301c1438:~$ 

```
 - Install the SDK (assuming it was in the mounted volume).
```
cd /home/user/hack
sudo apt install gaia-0.1.0_amd64.deb
```
 - Start the server running (preferrably in a separate ssh to container.
```
./run.sh
```
 - Build and run the hackathon demo.
```
cmake .
make
./hack
help
```
 - Attach with vscode. Install "Remote - SSH" extension. The configuration file (probably in `~/.ssh/config`) should contain:
```
Host localhost
    HostName localhost
    User user
    Port 2222
```

   Note that it will ask you to install `cppdbg` when trying to debug. Install the extension called
   `ms-vscode.cpptools`.
 - To clean out all data except for the catalog, run `./clear.sh`.
 - To force a complete new build and clean out the database, run `./reset.sh`.
