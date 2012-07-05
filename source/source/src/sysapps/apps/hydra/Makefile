include Defs.mak
## ############################################# ##
## Please put your -I and -D flags into Defs.mak ##
## ############################################# ##

OBJS	= hydra.o config.o encode.o mmm.o network.o signal.o umgr.o recv.o
OBJS	+= rtp/rtp.o rtp/rtcp.o
OBJS	+= tstamp.o timer.o
OBJS	+= log.o
OBJS	+= fps/fps.o
OBJS	+= watchdog/watchdog.o


ifeq	($(AUDIO),yes)
OBJS	+= ./audio/pcm_source.o ./audio/g726.o ./audio/g711.o ./audio/rtp_audio.o
OBJS	+= ./wave/decode.o ./wave/g726.o
endif

ifeq	($(ASF),yes)
OBJS	+= ./asf/asf_header_object.o ./asf/asf_data_object.o ./asf/asf_network.o ./asf/asf_common.o
endif

ifeq	($(MOTION_JPG),yes)
OBJS   += ./encode/JPEGHeader.o ./encode/jpg_qtbl.o
OBJS   += ./jpeg/jpeg.o
endif

ifeq	($(MPEG4),yes)
OBJS	+= ./encode/volhdr.o
endif

ifeq	($(SNAPSHOT),yes)
OBJS	+= ./snapshot/snapshot.o
SNAPSHOT_O	= ./snapshot/snapshooter.o
SNAP_SHOOTER   =   snapshooter
endif

ifeq    ($(SNAP_SWENCODE_FUN),yes)
SNAP_SWENCODE   = snap_swencoder
SNAP_SWENCODE_O = snap_swencode/main.o snap_swencode/encoder.o snap_swencode/dct.o snap_swencode/huffman.o snap_swencode/marker.o snap_swencode/quant.o snap_swencode/readYUV.o snap_swencode/img_scaling400_avg.o snap_swencode/img_scaling420_avg.o snap_swencode/img_scaling422_avg.o  snap_swencode/img_gen_yuv.o snap_swencode/img_scaling400_skip.o snap_swencode/img_scaling422_skip.o
endif

HYDRA_DIAGNOSIS   = hydra_diagnosis_a
HYDRA_DIAGNOSIS_O = hydra_diagnosis/hydra_diagnosis_audio.o hydra_diagnosis/hydra_diagnosis_main.o hydra_diagnosis/hydra_diagnosis_mobile.o hydra_diagnosis/hydra_diagnosis_mpeg1.o hydra_diagnosis/hydra_diagnosis_shm.o hydra_diagnosis/hydra_diagnosis_tstamp.o hydra_diagnosis/hydra_diagnosis_users.o hydra_diagnosis/hydra_diagnosis_if.o

ifeq	($(MOTION_DETECT),yes)
OBJS	+= ./md/md.o ./md/mot_setup.o ./md/main_mot.o ./md/mot_algorithm.o ./md/mot_debug.o ./md/mot_getinfo.o
endif

ifeq	($(ONSCREENDISPLAY),yes)
OBJS	+= ./osd/osd.o
endif

ifeq	($(EVENT),yes)
EVENT_LIB = ./event/libevent.a
IN_DETECT = ./input_detect/input_detect
LIBS += -L./event -levent -L../cgi2/bin
endif


ifeq ($(MCAST_SAP_FUN),yes)
OBJS	+= ./mcast_sap/mcast_sap.o
endif

OBJS	+= ./rtsp/rtsp.o ./rtsp/parse.o ./rtsp/network.o ./rtsp/util.o ./rtsp/sdp.o ./rtsp/response.o ./rtsp/checkip.o ./rtsp/pv_spec.o


LDFLAGS =   -s


LIBS	+= -L../cgi2/bin -lpond
LIBS    += libio.a  -lpthread

INSTPROGS   =   hydra

all:    $(INSTPROGS) $(SNAP_SHOOTER) $(SNAP_SWENCODE) $(IN_DETECT) $(HYDRA_DIAGNOSIS) install 


$(HYDRA_DIAGNOSIS): $(HYDRA_DIAGNOSIS_O)
	@$(CC) $(LDFLAGS) -o $@ $^  libio.a

$(INSTPROGS): $(OBJS) $(FTPOBJ) build_event 
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(FTPOBJ) $(LIBS)

$(SNAP_SHOOTER): $(SNAPSHOT_O) 
	$(CC)  $(CFLAGS) -o $@ $^


$(SNAP_SWENCODE): $(SNAP_SWENCODE_O)
	@$(CC) $(LDFLAGS) -o $@ $^ 




ifeq	($(EVENT),yes)
$(IN_DETECT):
	$(MAKE) --no-print-directory -C input_detect

build_event:
	$(MAKE) --no-print-directory -C event
endif

	
clean:	uninstall
	rm -rf $(OBJS) $(SNAPSHOT_O) *~ $(INSTPROGS) $(SNAP_SHOOTER)  $(SNAP_SWENCODE) $(INSTPROGS)_maps.txt
	rm -rf $(SNAP_SWENCODE_O) $(HYDRA_DIAGNOSIS) $(HYDRA_DIAGNOSIS_O)
	rm -rf $(EVENT_LIB)
	$(MAKE) --no-print-directory -C event $@
	$(MAKE) --no-print-directory -C input_detect $@

install:
	#@$(OBJDUMP) -S -s -d  $(INSTPROGS) > $(INSTPROGS)_maps.txt
	@$(STRIP)  --remove-section=.note --remove-section=.comment $(INSTPROGS)
	@install -D $(HYDRA_DIAGNOSIS) $(PROJ_INSTALL)/usr/local/bin/$(HYDRA_DIAGNOSIS)
	@install -D $(INSTPROGS) $(PROJ_INSTALL)/usr/local/bin/$(INSTPROGS)
	@install -D $(SNAP_SHOOTER) $(PROJ_INSTALL)/usr/local/bin/$(SNAP_SHOOTER)
	@install -D $(SNAP_SWENCODE) $(PROJ_INSTALL)/usr/local/bin/$(SNAP_SWENCODE)
	@install -D $(IN_DETECT) $(PROJ_INSTALL)/usr/local/bin/input_detect
	@install -D killone $(PROJ_INSTALL)/usr/local/bin/killone
	@ln -sf /tmp/vlog/file $(PROJ_INSTALL)/usr/local/www/vlog

uninstall:
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(INSTPROGS)
	@rm -f $(PROJ_INSTALL)/usr/local/bin/$(SNAP_SHOOTER)

gpl-clean:
	find . -name "*.c"  | xargs rm -rf
	rm -rf ./event/*.h
