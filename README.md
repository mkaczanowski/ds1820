[![Build Status][travis-badge]][travis]

[travis-badge]: https://travis-ci.org/mkaczanowski/ds1820.svg?branch=master
[travis]: https://travis-ci.org/mkaczanowski/ds1820/

# ds1820
The project describes how to build a leightweight C HTTP server (based on [libmicrohttpd](http://www.gnu.org/software/libmicrohttpd/ "libmicrohttpd")) that:
* periodically collects temperature via [DS182B20](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf "DS182B20") sensors
* stores measurements in the [sqlite3](https://www.sqlite.org/index.html "sqlite3") database
* exposes data via [jQuery mobile web interface](https://jquerymobile.com/ "jQuery mobile web interface")
* notifies the user if sensors go out of range ([msmtp](http://msmtp.sourceforge.net/ "msmtp") required)

In addition to the above, I described the process of wiring and system setup with [BeagleBone Black](http://beagleboard.org/bone "BeagleBone Black") board.

## Wiring
The ARM board communicates with the sensors via [1-wire serial protocol](https://www.maximintegrated.com/en/design/technical-documents/tutorials/1/1796.html "1-wire serial protocol") that supports multiple devices on one data line. To enable the driver, we'll have to instruct the kernel about physical wiring, so you should make sure the correct GPIO pin is present in the DTS (device tree source) file.

The figure below presents example setup specifically for BeagleBone Black board (P9.22 GPIO pin, 4.7 kOhm resistor), but it should also work for other prototype boards (ie. Arduino):

[![](http://mkaczanowski.com/wp-content/uploads/2015/01/beagle-1-1024x675.png)](http://mkaczanowski.com/wp-content/uploads/2015/01/beagle-1.png)

## System setup
Not so recently, kernel overlays have been deprecated, so anything that depends on "bone_capemgr" [won't work](https://github.com/beagleboard/linux/issues/139 "won't work"). Instead, we must configure/load the device tree overlays [at the boot time with U-Boot](https://elinux.org/Beagleboard:BeagleBoneBlack_Debian#U-Boot_Overlays "at the boot time with U-Boot").

At first we need to compile the DTS file:
```shell
$ git clone https://github.com/mkaczanowski/ds1820
$ cd dts && dtc -O dtb -o BB-W1-00A0.dtbo -b 0 -@ BB-W1-00A0.dts 
$ cp BB-W1-00A0.dtbo /lib/firmware/
```

Now, the `dtbo` file needs to be loaded via U-Boot:
```shell
$ echo "enable_uboot_overlays=1" >> /boot/uEnv.txt
$ echo "uboot_overlay_addr4=/lib/firmware/BB-W1-00A0.dtbo" >> /boot/uEnv.txt
```

If all goes right, after the reboot you should see sensors exposed via sysfs:
```shell
$ modprobe wire
$ modprobe w1-gpio
$ ls /sys/bus/w1/devices/
28-0000035f5e27  28-000004b58c5d  w1_bus_master1
```

Note that the U-Boot configuration is tightly coupled with the BeagleBone Black official Debian image and likely won't be the same on other systems.

## Building
It's a standard CMake project, so everything should build smoothly. However dependencies such as `libmicrohttpd` or `sqlite3` must be installed manually (via apt).
```shell
$ git clone https://github.com/mkaczanowski/ds1820
$ cd ds1820

$ mkdir build && cd build
$ cmake ../ && make
```

The server is configured via JSON config, where you have to update sensors sysfs paths:
```json
{
  "host": "0.0.0.0",
  "port": 8888,
  "db_path": "test.db",
  "db_update_rate_secs": 10,
  "email_send_rate_secs": 600,
  "email_recipient": "test@test.com",
  "temperature_scale": 0,
  "sensors_range": {
        "min": -5,
        "max": 17
  },
  "sensors": [
        "/sys/bus/w1/devices/28-0000035f5e27/w1_slave",
        "/sys/bus/w1/devices/28-000004b58c5d/w1_slave"
  ]
}
```

Spawn up the server:
```shell
$ ./temperature-server ../config.json
```

## Web interface
[![](http://mkaczanowski.com/wp-content/uploads/2015/01/web-front.png)](http://mkaczanowski.com/wp-content/uploads/2015/01/web-front.png)

[![](http://mkaczanowski.com/wp-content/uploads/2015/01/web-back.png)](http://mkaczanowski.com/wp-content/uploads/2015/01/web-back.png)
