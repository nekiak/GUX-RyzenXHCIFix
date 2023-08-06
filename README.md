## GUX-RyzenXHCIFix

This is a fork of GenericUSBXHCI aimed at analyzing and fixing the USB3 issue found on some Ryzen APU-based hackintoshes running macOS 11.0+
An experimental kext which fixed the issue on my own laptop (Picasso APU) can be downloaded on the releases tab, *but you should **really** read through this document before using it (especially "the warning").*

### The USB issue

A number of Ryzen APU-based hackintoshes hang on boot while initializing their XHCI controllers due to a conflict caused by a still not entirely understood issue. While it is possible to use GenericUSBXHCI (GUX) as a workaround on Catalina and gain back full USB functionality, Apple has made significant changes to the way macOS handles USB starting from Big Sur, which in turn ended up breaking several of GUX's functions (it would still allow you to boot, but USB mass storage and many other composite devices such as webcams do not work). An alternative work-around to using GUX on 11.0+ was to use [Smokeless UMAF](https://github.com/DavidS95/Smokeless_UMAF) to disable one of the XHCI controllers, however, not all devices have said option avaliable on UMAF, and even if it is available, it would mean disabling either the device's external USB ports or internal USB devices (webcam/bluetooth/etc).

### The "fix"

While looking for a fix, I've noticed booting with GUX installed and using the -gux_nomsi boot-arg would make USB work flawlessly on my system. I've then shared my findings with [Noot Inc.](https://nootinc.github.io/) and, while analyzing what happened with [Visual Ehrmanntraut](https://github.com/ChefKissInc) and Yan Schafer, we noticed this was because GUX would get loaded and perform some of its USB init functions, then, because my laptop had no way of using USB without MSI, it would error out and quit during early boot, AppleUSBXHCIPCI to attach to XHC0 and XHC1 instead. With this info in mind, I've then forked GUX, made some small changes allowing it to compile under Xcode 14.3.1 (target Ventura 13.5) and forced it to always exit where it would normally error out with -gux_nomsi, so not only no boot-args are needed, but it should potentially make the kext work on systems where the issue is present but USB without MSI is supported properly.

### The warning

We are not fully sure what exactly in GUX's early init code solves the XHCI conflict yet, all we know is *something* there fixed it on this device. As such, this kext should **not** be considered a proper, definitive solution, and the issue should be looked into more thorougly. Given the underlying complexity causing this issue, there's also no guarantee this modded kext will solve the XHCI conflict on your system as well. I'm only providing the compiled kext as a temporary workaround.

The goal of this project is to hopefully identify and isolate what makes XHCI work properly on 11.0+ Ryzen APU hackintoshes and make a separate kext out of it ~~if i have enough free time.~~

### Disclaimer

I'm not affiliated nor part of Noot Inc., AMD OSX or any other hackintosh dev group, you should not nag them if you encounter issues with this kext. I'm just a hobbyist dev who decided to share my findings as I was affected by the USB3 issue personally.
I also do not guarantee I'll keep working on this project as I might not have enough free time to do so.

### History

This repository contains a modified version of Zenith432's GenericUSBXHCI USB 3.0 driver. All credits to Zenith432 for the original code and probably further enhancements/bug fixes.

Original sources came from this post on InsanelyMac:

http://www.insanelymac.com/forum/topic/286860-genericusbxhci-usb-30-driver-for-os-x-with-source/

Original repo:
http://sourceforge.net/p/genericusbxhci/code
