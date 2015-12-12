# really just some handy scripts...

KEXT=GenericUSBXHCI.kext
DIST=RehabMan-Generic-USB3

#OPTIONS=LOGNAME=$(LOGNAME)

ifeq ($(findstring 32,$(BITS)),32)
OPTIONS:=$(OPTIONS) -arch i386
endif

ifeq ($(findstring 64,$(BITS)),64)
OPTIONS:=$(OPTIONS) -arch x86_64
endif

INSTALLDIR=Universal
#ifeq ($(OSTYPE),"darwin14")
#INSTALLDIR=Yosemite
#endif

.PHONY: all
all:
	xcodebuild clean $(OPTIONS) -configuration "El Capitan"
	make -f xhcdump/xhcdump.mak

.PHONY: clean
clean:
	xcodebuild clean $(OPTIONS) -configuration "El Capitan"
	rm ./xhcdump/xhcdump

.PHONY: update_kernelcache
update_kernelcache:
	sudo touch /System/Library/Extensions
	sudo kextcache -update-volume /

.PHONY: install
install:
	sudo cp -R ./Build/$(INSTALLDIR)/$(KEXT) /Library/Extensions
	make update_kernelcache

.PHONY: distribute
distribute:
	if [ -e ./Distribute ]; then rm -r ./Distribute; fi
	mkdir ./Distribute
	#cp -R ./Build/Legacy ./Distribute
	#cp -R ./Build/Mavericks ./Distribute
	#cp -R ./Build/Yosemite ./Distribute
	#cp -R ./Build/Universal ./Distribute
	#cp -R ./Build/El\ Capitan ./Distribute
	cp ./xhcdump/xhcdump ./Distribute
	find ./Distribute -path *.DS_Store -delete
	find ./Distribute -path *.dSYM -exec echo rm -r {} \; >/tmp/org.voodoo.rm.dsym.sh
	chmod +x /tmp/org.voodoo.rm.dsym.sh
	/tmp/org.voodoo.rm.dsym.sh
	rm /tmp/org.voodoo.rm.dsym.sh
	ditto -c -k --sequesterRsrc --zlibCompressionLevel 9 ./Distribute ./Archive.zip
	mv ./Archive.zip ./Distribute/`date +$(DIST)-%Y-%m%d.zip`
