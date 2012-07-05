INSTALL_DIR	= $(PROJ_INSTALL)/usr/local/www/adm
CFLAGS  += -D_ENCRYPT_CFG_EN_

ifeq ($(OEM),Neutral)
CFLAGS += -D_PAN_TILT_SUPPORT_
endif

include $(PROJ_CGI2)/makefiles/makefile_exe
	
