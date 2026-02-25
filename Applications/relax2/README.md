## Installing relax2

To get started with relax2, enter the folder and
```
poetry install
```
then run with
```
poetry run python3 Relax2_main.py
```
Make sure that the xcb plugin is installed 
```
sudo apt-get install qt5dxcb-plugin
```
## trouble shooting your installation

you might run into this problem:

qt.qpa.plugin: Could not load the Qt platform plugin "xcb" in "" even though it was found.
This application failed to start because no Qt platform plugin could be initialized. Reinstalling the application may fix this problem.

you can debug this with:
```
QT_DEBUG_PLUGINS=1 poetry run python3 Relax2_main.py
```
Likely you will find that the xcb plugin is not installed correctly.

To use the latest development version of Relax2.0 you need to update the relax2 folder on your host computer (here a RaspberryPi) and the server and binaries files on the Red Pitaya 125-14 (see server folder README)!!!

## Setup a Raspberry Pi5 as host Computer

Setup: Raspberry Pi 5, 32GD micro SD card, direct connection (LAN) between Raspberry Pi and Red Pitaya

### Install Raspberry Pi OS to the micro SD card

Micro SD card in/on your computer (direct, SD card adapter, USB dongle):

Download Raspberry Pi Imager (https://www.raspberrypi.com/software/).

Raspberry Pi Imager Software:

- Device: Raspberry Pi 5
- OS: Raspberry Pi OS (64bit)
- Storage: Select your SD Card
- Customisation: Skip

### Raspberry Pi setup

Micro SD card in Raspberry Pi 5:

Boot up the raspberry and finish the initial setup.

- Username (default): pi
- Password (default): raspberry

Make sure you have a working internet connection and port 123 is not blocked. 
This is confirmed when the timesync and update at the end of the initial setup is successful.

Setup the LAN port IPs for the Raspberry and the Red Pitaya:

Open the terminal (black box in the task list or Ctrl + Alt + T)

#### Fixed IP for the Raspberry Pi:

Terminal: 
```
sudo nmcli con add type ethernet con-name "RedPitaya-LAN" ifname eth0 ip4 192.168.1.1/24
```
#### DHCP-Server to provide IP for the Red Pitaya:

Terminal: 
```
sudo apt update
sudo apt install dnsmasq
sudo mv /etc/dnsmasq.conf /etc/dnsmasq.conf_bak
sudo nano /etc/dnsmasq.conf
```

In Nano editor append to end of file:
```
interface=eth0
dhcp-range=192.168.1.84,192.168.1.84,255.255.255.0,24h
```

Save file: Ctrl + O -> Enter
Exit Nano editor: Ctrl + x

#### Configurate the Raspberry Pi:

Terminal:
```
sudo raspi-config
```
In Raspberry Pi configuration window:

Advanced Options -> Wayland -> X11
System Options -> Auto Login -> Console: yes -> Desktop: no

Confirm with Finish

#### Install Python packages:

Terminal:
```
sudo apt-get update
sudo apt-get install python3-pyqt5.qtserialport
sudo apt-get install python3-matplotlib
sudo apt-get install qttools5-dev-tools qt5-assistant
```
Copy the current relax2 folder in the Home folder of the Raspberry Pi (USB stick).

Open the Relax2_main.py with Thonny and run it.
