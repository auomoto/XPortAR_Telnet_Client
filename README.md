XPort AR Telnet Client
======================

This program makes a telnet connection to a Lantronix XPort AR Ethernet embedded network processor module and communicates with its "Line 1" serial port. The purpose is to learn how to command serial devices over the network using the XPort AR.

This particular example talks to my ATtiny4313_Servo board (see the github entry). It moves the servo motor to random positions before exiting.

This program compiles under gcc in the Windows 8.1 Cygwin environment and runs in a Windows 8.1 Cygwin terminal window.

To set up the Lantronix XPort AR:
- Download and install the Lantronix DeviceInstaller (I used version 4.4.0.0)
- Start the DeviceInstaller and do a "Search." This will take a while but you'll eventually get an xPort icon. Click through to the auto-assigned IP address and click on it.
- Take a look at the Device Details.
- Click on "Assign IP" and put in your new IP address. For my home network (LinkSys router) I chose 192.168.1.99.
- The progress bar won't show anything if your computer is not on the new subnet. Change your host computer IP address to the new subnet.
- I used PuTTY to confirm an initial telnet connection.
