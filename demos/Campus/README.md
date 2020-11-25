# Campus Demo
yada ...

# Prerequisites
Install gtest ( https://github.com/google/googletest )

for campus_demo_term install ncurses ( sudo apt-get install libncurses5-dev libncursesw5-dev )

## Build Instructions
1. Build Gaia Platform production code.
2. yada ...






# Appendix

- Installing gtest

sudo apt-get install libgtest-dev
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib

- ADLink environment (for Edge demo)

get a license file from ADLink, put it here: /media/mark/Data1/U20/SDK/ADLINK/EdgeSDK/1.5.1/etc/license.lic

put these lines in ~/.bashrc

export ADLINK_LICENSE=file:///media/mark/Data1/U20/SDK/ADLINK/EdgeSDK/1.5.1/etc/license.lic
export ADLINK_DATARIVER_URI=file:///media/mark/Data1/U20/SDK/ADLINK/EdgeSDK/1.5.1/etc/config/default_datariver_config_v1.6.xml