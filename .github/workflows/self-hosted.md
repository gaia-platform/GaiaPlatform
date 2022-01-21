# Self Hosted Build Machines

Right now, to get things off the ground, this is a manual process to set up the build machines.
We probably will not have to set up new build machines that often/
As such, it is a question of priorities as to whether to automate this process more, or try and make it more scriptable.

## Get Access to Azure From Matthew

Before you can start doing anything with the build machines, you will need Azure access from Matthew.
Once you have that, make sure you can login to the [Azure portal](https://portal.azure.com/) and look around.

To simply look at the build machines and their status, no further action is required.
To actively SSH into the build machines, only the next step is required.
The remaining steps should be followed to set up a new build machine.
Any failure to set up the machine properly will likely result in either the machine not being available for building, or not properly handling the build jobs as they become available.

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
  - note that this link talks about AWS, but the same thing applies to Azure

## Creating the VM

Go to the home page for your Azure account, and search for `Virtual Machines`.
On that page, click on the `Create` button at the top of the list of available machines.

In the `Basics` tab, set the following options as indicated here:
- `resource group`: set to `build-1_group`
- `virtual machine name`: set to something unique, but preface with `builder-`
- `image`: `ubuntu 20.04 LTS`
- `size`: `Standard D4s v3 (4 vcpus, 16 GiB memory)`
- `ssh public key source`: `use existing key stored in Azure`
- `stored keys`: `build-machine-key`

In the `Disks` tab, create and attach a new disk.
For that new disk, add the disk with the following options:
- `number`: `1`
- `size`: `64 GB`

Once those are done, press the button that says `Review and Create`.
Carefully review everything on the page to make sure it looks right, then press the `Create` button.
The deploy might take a while to complete, displaying `Deployment in process` during the entire time.

After around one minute, it should indicated that it finished deploying the Virtual Machine, and present a button that says `go to resource`.
Pressing that button will take you to the main page for the newly create VM.
Of particular interest is the IP address on the right under Public IP Address.
That address is needed to connect to the build machine, so note it for later use.

- Reference: [Using the GitHub self-hosted runner and Azure Virtual Machines to login with a System Assigned Managed Identity](https://www.cloudwithchris.com/blog/github-selfhosted-runner-on-azure/)

## Connecting to the VM

On the virtual machines page for the specific machine you want to login to, there is a Networking section on the right.
Note the IP address for the VM from the `Public IP address` field.

To SSH from your local shell to the build machine, use the following form:

```sh
ssh azureuser@<ip address>
```

### Attaching the Data Drive

Once you have SSHed in to the build machine, the first thing to do is to attach the data drive to the virtual machine.
The first step of this is to use this command:

```sh
lsblk -o NAME,HCTL,SIZE,MOUNTPOINT | grep -i "sd"
```

to locate the data drive.
Unless something changes, the new drive should be listed at the end as `sdc`, indicate a 64G drive, and no mount point.
If this changes, please talk to someone else and get verification that things look correct.

Assuming things look right, the first step to using the disk is to partition it with the following commands:

```sh
sudo parted /dev/sdc --script mklabel gpt mkpart xfspart xfs 0% 100%
sudo mkfs.xfs /dev/sdc1
sudo partprobe /dev/sdc1
```

and then mount the partition with these commands:

```sh
sudo mkdir /datadrive
sudo mount /dev/sdc1 /datadrive
```

To verify that things are set up properly, execute:

```sh
lsblk -o NAME,HCTL,SIZE,MOUNTPOINT | grep -i "sd"
```

This time, at the end, you should see:

```text
sdc     1:0:0:1      64G
└─sdc1               64G /datadrive
```

At this point, the data drive is partitioned and mounted, but will require re-mounting if the virtual machine is restarted.
To solve that issue, we need to persist the mounting.
The first step of that process is to execute the following command:

```sh
sudo blkid
```

Near the bottom of the output should be a line for the mount that we recently added, `/dev/sdc1`.
The important piece of information to keep track of is the UUID associated with the mount, like `2ac03c2e-da59-4bd3-b9ad-a9ceaea60a01`.
To persist the mount, edit the `/etc/fstab` file with"

```sh
sudo nano /etc/fstab
```

and add the following line to the end of the file:

```
UUID=2ac03c2e-da59-4bd3-b9ad-a9ceaea60a01   /datadrive   xfs   defaults,nofail   1   2
```

but replace the UUID `2ac03c2e-da59-4bd3-b9ad-a9ceaea60a01` with the one that you noted from the call to `blkid`.
If you are not familiar with `nano`, press `Ctrl-S` to save and then `Ctrl-X` to exit.

Finally, to make sure we do not run into permissions problems anywhere else, run the following commands to allow anything inside of our Virtual Machine to access the drive:

```
sudo chmod 777 /datadrive
mkdir /datadrive/docker
mkdir /datadrive/runner
```

- Reference: [Use the portal to attach a data disk to a Linux VM](https://docs.microsoft.com/en-us/azure/virtual-machines/linux/attach-disk-portal)

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

Finally, to verify that everything has been setup correctly, it is recommended that you execute this command:

```sh
docker run hello-world
```

- Reference: [Install Docker Engine on Ubuntu](https://docs.docker.com/engine/install/ubuntu/)

## Move Docker Data Directory

Now that Docker is installed, we need to move its data directory to the data drive.
To do this, we first stop the docker service:

```sh
sudo service docker stop
```

Once the service is stopped, we need to create a new file using:

```sh
sudo nano /etc/docker/daemon.json
```

and add the following content for that file:

```text
{
  "data-root": "/datadrive/docker"
}
```

If you are not familiar with `nano`, press `Ctrl-S` to save and then `Ctrl-X` to exit.

To be on the safe side, once we have saved the file, run the command:

```sh
sudo chmod 777 /etc/docker/daemon.json
```

to set global permissions for this file.

At this point, we have the data directory for Docker moved to its new location, but we also need to move the previous information over.
To do this, execute the following commands to copy the configuration over to the new directory and to make a backup of the old information:

```sh
sudo rsync -aP /var/lib/docker/ /datadrive/docker
sudo mv /var/lib/docker /var/lib/docker.old
```

When you are ready, the next step is to start Docker up using the following command:

```sh
sudo service docker start
```

At this point, the Docker service should start up without any problems.
Another good check is to see if the `hello-world` image can be spun up again.
Once you are satisfied that Docker is running properly, you can clean up the old Docker data directory using:

```sh
sudo rm -rf /var/lib/docker.old
```

If you would like, you can keep that information around until after a couple of build jobs have successfully passed.

- Reference: [HOW TO MOVE DOCKER DATA DIRECTORY TO ANOTHER LOCATION ON UBUNTU](https://www.guguweb.com/2019/02/07/how-to-move-docker-data-directory-to-another-location-on-ubuntu/)

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

When prompted for the name of the machine, it should automatically start with `builder-` to match the name of the machine, but double check that.
During the execution of the workflow, the extra "little bits" required by the workflow to work efficiently with our build machines key off of this.
An example of this is:

```sh
    if [[ "${{ runner.name }}" =~ ^builder.* ]] ; then
      docker system prune --force
    else
      echo "Setting up SSH socket for Docker."
      ssh-agent -a $SSH_AUTH_SOCK > /dev/null
    fi
```

## Issues

### Debian Package Being Written with Root Permissions

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
