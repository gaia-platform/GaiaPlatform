# Self Hosted Build Machines

## Get Access to Azure From Matthew

Before you can start doing anything with the build machines, you will need Azure access from Matthew.
Once you have that, make sure you can login to the [Azure portal](https://portal.azure.com/) and look around.

## Azure SSH Key

This key is stored in Azure and is a one-time download thing.
This key is required to allow SSH access into the virtual machines that host the build.

Currently, contact Jack to get it.
Note: Working with Matthew to find a permanent location for it.

Before going any further, download the PEM to your machine, and add it to your keychain, using something similar to:

```sh
ssh-add ~/keyfile.pem
```

- Reference: [How to Add Your EC2 PEM File to Your SSH Keychain](https://www.cloudsavvyit.com/1795/how-to-add-your-ec2-pem-file-to-your-ssh-keychain/)

## Creating the VM

(This is still a work in progress.)

Search for `Virtual Machines`
- Create button at top

Basics tab
- resource group-> build-1_group
- virtual machine name-> unique, preface with `build-`
- image -> ubuntu 20.04 LTS
- size -> Standard D4s v3 (4 vcpus, 16 GiB memory)
- ssh public key source -> use existing key stored in Azure
- stored keys -> build-machine-key

Disks tab
- Create and attach a new disk
- add disk 64GB, Lun-1

Review and Create
- review everything and then press Create
- might take a while to complete, will see `Deployment in process`

Press "go to resource"

Notice the IP address on the right under Public IP Address

- Reference: [Using the GitHub self-hosted runner and Azure Virtual Machines to login with a System Assigned Managed Identity](https://www.cloudwithchris.com/blog/github-selfhosted-runner-on-azure/)

## Connecting to the VM

On the virtual machines page for the specific machine you want to login to, there is a Networking section on the right.
Note the IP address for the VM from the `Public IP address` field.

```sh
ssh azureuser@<ip address>
```

### Attaching the Data Drive

- find drive

```sh
lsblk -o NAME,HCTL,SIZE,MOUNTPOINT | grep -i "sd"
```

- should be listed as `sdc` with a 64G drive
- note that it is not mounted

- partition the disk
```sh
sudo parted /dev/sdc --script mklabel gpt mkpart xfspart xfs 0% 100%
sudo mkfs.xfs /dev/sdc1
sudo partprobe /dev/sdc1
```

- mount
```sh
sudo mkdir /datadrive
sudo mount /dev/sdc1 /datadrive
```

- persist the mount
```
sudo blkid
```
- note the UUID for the mount `/dev/sdc1`, like `2ac03c2e-da59-4bd3-b9ad-a9ceaea60a01`
- should be at the bottom

```
sudo nano /etc/fstab
```

add to end
```
UUID=2ac03c2e-da59-4bd3-b9ad-a9ceaea60a01   /datadrive   xfs   defaults,nofail   1   2
```

execute again to verify things are set up
```
lsblk -o NAME,HCTL,SIZE,MOUNTPOINT | grep -i "sd"
```

at the end, should see:
```
sdc     1:0:0:1      64G
└─sdc1               64G /datadrive
```

```
sudo chmod 777 /datadrive
mkdir /datadrive/docker
mkdir /datadrive/runner
```

- Reference: [xx](https://docs.microsoft.com/en-us/azure/virtual-machines/linux/attach-disk-portal)

## Install Docker

When the machines are first spun up, they are clean versions of the Ubuntu 20.04 system.
As such, Docker is not installed on those machines and needs to be installed for the build machines.

```sh
sudo apt-get update
sudo apt-get install \
    ca-certificates \
    curl \
    gnupg \
    lsb-release
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
sudo apt-get -y install docker-ce docker-ce-cli containerd.io
```

Once those steps are performed, it is recommended to run the following command to verify that the installation succeeded.

```
sudo service docker stop
sudo chmod 666 /var/run/docker.sock
sudo service docker start
```

```sh
docker run hello-world
```

- Reference: [Install Docker Engine on Ubuntu](https://docs.docker.com/engine/install/ubuntu/)

## Move Docker Data Directory

sudo service docker stop

- `sudo nano /etc/docker/daemon.json`
```
{
  "data-root": "/datadrive/docker"
}
```
sudo chmod 777 /etc/docker/daemon.json

- copy docker over
```
sudo rsync -aP /var/lib/docker/ /datadrive/docker
```

- make backup
```
sudo mv /var/lib/docker /var/lib/docker.old
```

```
sudo service docker start
```

- after double check with hello-world, cleanup:
```
sudo rm -rf /var/lib/docker.old
```

- Reference: [x](https://www.guguweb.com/2019/02/07/how-to-move-docker-data-directory-to-another-location-on-ubuntu/)

## Add User to Docker Group

With Docker now installed, the `azureuser` needs to be added to the `docker` group.
This is required to allow that user to be able to submit Docker commands within the workflows.

```sh
sudo usermod -a -G docker azureuser
```

- Reference: [Add a User to a Group (or Second Group) on Linux](https://www.howtogeek.com/50787/add-a-user-to-a-group-or-second-group-on-linux/)

## Adding The GitHub Runner

With all the foundations set, the last thing to do is to follow the instructions from the repository to install a new self-hosted runner.
By going to [this page](https://github.com/gaia-platform/GaiaPlatform/settings/actions/runners/new), you will find detailed instructions on how to set up the runner.
Note that these instructions include information that is keyed to our repository and must be entered in exactly as specified on that page.
To help in this, each section of the installation scripts has a *Copy to Clipboard* action that you can use by just clicking on the item.

When running the `config.sh` script, most of the prompts should be answered with no changes.
For the `Enter name of work folder`, enter `/datadrive` to match the mounted data drive.

## Issues

```
Run actions/checkout@master
Syncing repository: gaia-platform/GaiaPlatform
Getting Git version info
Deleting the contents of '/datadrive/runner/GaiaPlatform/GaiaPlatform'
Error: Command failed: rm -rf "/datadrive/runner/GaiaPlatform/GaiaPlatform/build"
rm: cannot remove '/datadrive/runner/GaiaPlatform/GaiaPlatform/build/output/package/gaia-0.3.3_amd64.deb': Permission denied
```
- seems to be a permissions issue
  - added `sudo chmod -R 777 /datadrive/runner/GaiaPlatform/GaiaPlatform/build` to start of jobs