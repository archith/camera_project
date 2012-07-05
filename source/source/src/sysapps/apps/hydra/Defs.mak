APPS_DIR	= $(PROJ_ROOT)/src/sysapps/apps
HYDRA_DIR	= $(APPS_DIR)/hydra


include $(HYDRA_DIR)/Rules.mak

CROSS   =arm-linux-
CC      =$(CROSS)gcc
LD      =$(CROSS)ld
AR	=$(CROSS)ar
STRIP	=$(CROSS)strip
OBJDUMP =$(CROSS)objdump

CFLAGS  =   -Wall -Werror -march=armv4 -O2 -I$(HYDRA_DIR)/include -I$(PROJ_ROOT)/src/kernel/linux/include
CFLAGS	+= -I$(APPS_DIR)/cgi2/inc
CFLAGS  += -I$(HYDRA_DIR)/rtp/

CFLAGS	+= -I$(HYDRA_DIR)/jpeg/

ifeq    ($(DAEMON),yes)
CFLAGS 	+= -D_DAEMON_
endif

ifeq    ($(TEST),yes)
#CFLAGS	+=-D_DUMP_RAW_
endif

ifeq    ($(CCDC),yes)
CFLAGS  += -D_CCDC_
endif
ifeq	($(AUDIO),yes)
CFLAGS	+= -D_AUDIO_ 
CFLAGS  += -D_CONTROL_AUDIO_BUFFER_SIZE_
CFLAGS	+= -I$(HYDRA_DIR)/audio/

CFLAGS	+= -I$(HYDRA_DIR)/wave/
endif

ifeq	($(ASF),yes)
CFLAGS	+= -I$(HYDRA_DIR)/asf -D_ASF_
endif
ifeq	($(MOTION_JPG),yes)
CFLAGS	+= -D_MJPG_
ifeq    ($(TEST),yes)
CFLAGS	+= -D_DUMP_JPG_
endif
endif

ifeq	($(MPEG4),yes)
CFLAGS	+= -D_MP4V_
ifeq    ($(TEST),yes)
CFLAGS	+= -D_DUMP_FRAME_
endif
endif

ifeq	($(SNAPSHOT),yes)
CFLAGS	+= -D_SNAPSHOT_
endif

ifeq    ($(SNAP_SWENCODE_FUN),yes)
CFLAGS  += -D_SW_ENCODE_JPEG_
CFLAGS  += -D_SCALE_EN_
CFLAGS  += -D_YUV422_
CFLAGS  += -D_SCALE_FACTOR_FROM_SHMEM_
#CFLAGS += -D_SCALE_64X48_ #-D_SCALE_128X96_
CFLAGS  += -D_ALGORITHM_SKIP_ #-D_ALGORITHM_AVERAGE_
CFLAGS  += -D_SCALE_FACTOR_FROM_SHMEM_
#CFLAGS += -D_SWE_DEBUG_ -D_TEST_READ_FILE_YUV_  -D_OUTPUT_SCALED_YUV_
CFLAGS  += -Wall  -O2 -I$(PROJ_ROOT)/src/kernel/linux/include  -I$(HYDRA_DIR)/ -I$(HYDRA_DIR)/include  -I$(HYDRA_DIR)/rtp/  -I$(HYDRA_DIR)/rtsp/include -I$(HYDRA_DIR)/snap_swencode/
endif


ifeq	($(MOTION_DETECT),yes)
CFLAGS  += -D_MOTION_
#CFLAGS += -D_DEBUG_ #
#CFLAGS += -D_DB_AUTOFM_CRL_ 
#CFLAGS += -D_DUMP_MV_
CFLAGS	+= -D_AUTOFM_CRL_ -D_ALG_STAND_ -D_ALWAYS_INDCT #-D_LOG_TM_
CFLAGS  += -I$(HYDRA_DIR)/md/inc -I$(APPS_DIR)/ccdc_daemon
endif

ifeq	($(ONSCREENDISPLAY),yes)
CFLAGS	+= -D_OSD_
CFLAGS  += -I$(HYDRA_DIR)/osd
endif

ifeq	($(EVENT),yes)
CFLAGS  += -D_EVENT_THREAD_
CFLAGS	+= -DUSE_LOCK
CFLAGS  += -I$(HYDRA_DIR)/event
endif


ifeq ($(MCAST_SAP_FUN),yes)
CFLAGS += -D_MCAST_SAP_  -D_IPV4_
CFLAGS += -I$(HYDRA_DIR)/mcast_sap
endif


ifeq ($(MEM_64M),yes)
CFLAGS += -D_MEM_ABOVE_64M_
endif

CFLAGS  += -I$(HYDRA_DIR)/rtsp/include


#CFLAGS += -D_CALCULATE_CCD_FRAMERATE_

