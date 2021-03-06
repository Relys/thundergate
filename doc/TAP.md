# Thundergate TAP Driver #

The Thundergate toolkit includes Python modules inplementing a PCI driver for
Broadcom 57xx devices, and a TAP interface driver consuming it. Together they
comprise a userspace interface between the network hardware and the kernel
TCP/IP stack.

This functionality is available on Windows and Linux subject to the caveats
described in the platform-specific INSTALL files.

## Usage ##

Specify `-d` on the command line to start the execution of the driver.  The
driver runs in the foreground and offers a minimal single-key interface.

   ~~~
# python py/main.py -d
.
.
. 
[+] produced 128 rx buffers
[+] enabling transmit mac
[+] enabling receive mac
[+] configuring led
[+] waiting for interrupts...
[+] detecting link
[+] full duplex gige link negotated (res: 0000ff3c, txpause: False, rxpause: False)

d - link detect
h - help
q - quit
s - dump statistics
r - reset statistics
v - toggle verbosity
   ~~~

## Notes ##

 * The TAP interface created will assume the next available device name on the
host system, e.g. 'tapX' on Linux, or "TAP-Windows Adapter #X". It should only
persist for the life of the process, but the cleanup code is in `finally`
blocks, so crashed processes might leave behind dead interfaces requring manual
removal (especially on Windows).

 * The TAP device will bring itself up and down based on the state of the link,
but may need IP configuration using host OS tools (such as Linux's `ip` or
`ifconfig`,  or Windows's `ipconfig` or `ncpa.cpl`.)

 * Link detection and change notifications are hit-and-miss and may not trigger
at startup. Press `d` to force link re-negotiation.

* The TAP driver relies on asyncio from Python 3, as backported to Python 2
by the 'trollius' package (`pip install trollius`). This package has since 
been deprecated without replacement, although it continues to work.

## Performance ##

Performance was measured using iPerf3 over a point-to-point Cat5 cable
connecting a MacBook Air running Debian 8 and a MacBook Pro running Windows 10.
All compilation was performed without optimization and the standard CPython 2.7
interpreter was used. These (now outdated) numbers are only meaningful 
relatively (if even).

Server: Windows 10 w/ b57nd60a  
Client: Debian 8 w/ tg3  
Result:

   ~~~
[ ID] Interval           Transfer     Bandwidth       Retr
[  4]   0.00-10.00  sec   950 MBytes   797 Mbits/sec    0             sender
[  4]   0.00-10.00  sec   949 MBytes   796 Mbits/sec                  receiver
   ~~~

Server: Windows 10 w/ b57nd60a  
Client: Debian 8 w/ Thundergate TAP  
Result:

   ~~~
[ ID] Interval           Transfer     Bandwidth       Retr
[  4]   0.00-10.00  sec   151 MBytes   127 Mbits/sec    0             sender
[  4]   0.00-10.00  sec   150 MBytes   126 Mbits/sec                  receiver
   ~~~

Server: Debian 8 w/ tg3  
Client: Windows 10 w/ Thundergate TAP  
Result:

   ~~~
[ ID] Interval           Transfer     Bandwidth
[  4]   0.00-10.02  sec  11.8 MBytes  9.84 Mbits/sec                  sender
[  4]   0.00-10.02  sec  11.8 MBytes  9.84 Mbits/sec                  receiver
   ~~~

Server: Debian 8 w/ Thundergate TAP  
Client: Windows 10 w/ Thundergate TAP  
Result:

   ~~~
[ ID] Interval           Transfer     Bandwidth
[  4]   0.00-10.01  sec  17.1 MBytes  14.4 Mbits/sec                  sender
[  4]   0.00-10.01  sec  16.9 MBytes  14.2 Mbits/sec                  receiver
   ~~~
