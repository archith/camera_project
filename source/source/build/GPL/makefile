SOURCE = $(PROJ_ROOT)
TOOLCHAIN = $(PROJ_ROOT)/../toolchain
GPL_PACKAGE = $(PROJ_ROOT)/../GPL_PACKAGE
GPL_SOURCE = $(GPL_PACKAGE)/source/source/
GPL_TOOLCHAIN = $(GPL_PACKAGE)/source/toolchain

all:
ifeq ($(PROJ_ROOT),)
	@echo ERROR: Please go to root directory of source code and type \'soure setenv\' first
	exit 1
endif
	mkdir $(GPL_PACKAGE)
	mkdir $(GPL_PACKAGE)/source
	
	#copy source code
	mkdir $(GPL_SOURCE)
	cp -rf $(SOURCE)/* $(GPL_SOURCE)

	#copy toolchain source
	mkdir $(GPL_TOOLCHAIN)
	cp -rf $(TOOLCHAIN)/* $(GPL_TOOLCHAIN)
	
	#make 
	make -C $(SOURCE)
	
	#make gpl-clean
		#apps
		make -C $(SOURCE)/src/sysapps/apps gpl-clean
		rm -rf $(GPL_SOURCE)/src/sysapps/apps/*
		cp -rf $(SOURCE)/src/sysapps/apps $(GPL_SOURCE)/src/sysapps
		#kmods
		make -C $(SOURCE)/src/kmods gpl-clean
		rm -rf $(GPL_SOURCE)/src/kmods/*
		cp -rf $(SOURCE)/src/kmods $(GPL_SOURCE)/src
		#bootloader
		make -C $(SOURCE)/src/bootloader/proboot gpl-clean
		rm -rf $(GPL_SOURCE)/src/bootloader/proboot/*	
		cp -rf $(SOURCE)/src/bootloader/proboot $(GPL_SOURCE)/src/bootloader
		#FW
		make -C $(SOURCE)/FW gpl-clean
		rm -rf $(GPL_SOURCE)/FW/*	
		cp -rf $(SOURCE)/FW $(GPL_SOURCE)/
	
	#remove testing source
	rm -rf $(GPL_SOURCE)/src/bootloader/bootloader_document
	rm -rf $(GPL_SOURCE)/src/sysapps/apps/thttpd-2.25b
	rm -rf $(GPL_SOURCE)/src/sysapps/apps/throughput
	rm -rf $(GPL_SOURCE)/src/kmods/LSDK-WLAN-5.0.0-RC5-20051117
	rm -rf $(GPL_SOURCE)/src/kmods/tw9910
	rm -rf $(GPL_SOURCE)/src/kmods/plsensor
	
	#generate files
	cp -f license.doc $(GPL_PACKAGE)
	cp -f license.xls $(GPL_PACKAGE)/..
	cat readme.txt | sed 's/ENV_MODEL/$(MODEL)/' | sed 's/ENV_PROJ_DOMAIN/$(PROJ_DOMAIN)/'  > $(GPL_PACKAGE)/readme.txt
	
	
