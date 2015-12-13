## GenericUSBXHCI Fork by RehabMan & TheRacerMaster

### How to Install:

Install GenericUSBXHCI.kext using Kext Wizard or your favorite kext installer.

If you were previously using PXHCD.kext or a similar kext, you should probably remove it.
```
rm -rf /Library/Extensions/PXHCD.kext
```

### Downloads:

Downloads are available under GitHub Releases.

### Build Environment

This project requires Xcode 7 using the OS X 10.11 SDK, targeting OS X 10.11.

### Feedback:

Please use the following thread on insanelymac.com for feedback, questions, and help:
http://www.insanelymac.com/forum/topic/286860-genericusbxhci-usb-30-driver-for-os-x-with-source/

### Known issues:

- Might cause panics on 10.11/10.11.1; use 10.11.2+
- xHCI controllers using this driver will not show up under System Information/USB in 10.11

Support for xHCI controllers that have native support (Intel 7/8/9/100 Series / Fresco Logic FL1100) will eventually be removed from this driver.

### Change Log:

2015-12-13 (TheRacerMaster)

- Reverted IOKit USB headers to 10.9 version
- Added more errata for Fresco Logic FL1000 & VIA VL800/801
- Binary only works on 10.11 for now

2015-12-12 (TheRacerMaster)

- Added 10.10 IOKit USB headers to allow building & fix support on 10.11+
- Reorganized files in repo

2015-10-02 (RehabMan)

- The kext will now fail to load on 10.11+

2014-10-16 (RehabMan)

- Merged with latest Zenith432 version
- Created new Universal build for compatibility with 10.7.5 through 10.10

2013-03-23 (RehabMan)

- Modified for single binary to work on ML, Lion (10.7.5 only)
- Optimize build to reduce code size and exported symbols.

2013-03-06 (Zenith432)

- Initial build provided by Zenith432 on insanelymac.com

### History

This repository contains a modified version of Zenith432's GenericUSBXHCI USB 3.0 driver. All credits to Zenith432 for the original code and probably further enhancements/bug fixes.

Original sources came from this post on InsanelyMac:

http://www.insanelymac.com/forum/topic/286860-genericusbxhci-usb-30-driver-for-os-x-with-source/

Original repo:
http://sourceforge.net/p/genericusbxhci/code
