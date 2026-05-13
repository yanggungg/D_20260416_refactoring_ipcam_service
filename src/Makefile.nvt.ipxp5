#
# Makefile
#

TOPDIR = $(shell pwd)

-include make.env
-include Makefile.Rules

Q = @

##################################################

CROSS_COMPILE = /opt/aarch64-ca53-linux-gnueabihf-8.4.01/bin/aarch64-ca53-linux-gnu

AS      =$(CROSS_COMPILE)-as
LD      =$(CROSS_COMPILE)-ld
CC      =$(CROSS_COMPILE)-gcc
CPP     =$(CC) -E
AR      =$(CROSS_COMPILE)-ar
NM      =$(CROSS_COMPILE)-nm
STRIP   =$(CROSS_COMPILE)-strip
OBJDUMP =$(CROSS_COMPILE)-objdump
RANLIB	=$(CROSS_COMPILE)-ranlib

##################################################
#fisheye support
CXX=$(CROSS_COMPILE)g++

RELEASE = 0
MAJOR	= 1
MINOR	= 0

TARGET = nf_host
TARGET_SIMPLE = nf_simple_host
TARGET_WEBRA = webra.made

MACRO = -D_REENTRANT -D_LIBC_REENTRANT -DARCH_ARM 	\
		-DRELEASE="$(RELEASE)" -DMAJOR="$(MAJOR)" -DMINOR="$(MINOR)"

export LD_LIBRARY_PATH=/opt/aarch64-ca53-linux-gnueabihf-8.4.01/lib

#CFLAGS = -g -funwind-tables -fsigned-char -march=armv8-a -mtune=cortex-a53 -W -Wall \
#		-DX_LOCALE -D_IX_MEM_USE=0 -D_SKIP_CHECK_LICENSE=0 -DENABLE_DVR_AVI_PLAYER -DNO_PND=1 -funwind-tables -fasynchronous-unwind-tables\
#		-ffunction-sections -DCHIP_NVT_NT9833x -ftree-vectorize -fno-builtin -fno-common -Wformat=1

CFLAGS=-g -O2 -march=armv8-a -mtune=cortex-a53 -W -Wall \
        -DX_LOCALE -D_IX_MEM_USE=0 -D_SKIP_CHECK_LICENSE=0 \
        -DENABLE_DVR_AVI_PLAYER -DNO_PND=1 -funwind-tables \
        -fasynchronous-unwind-tables -DCHIP_NVT_NT9833x -fsigned-char -fno-common -ftree-vectorize -ffunction-sections -Wformat=1
#	-include mem_debug.h -DDEBUG_LEAK -D_GNU_SOURCE


# include for NFDVR
#INCDIR = -I$(NFDVR)/src/include \
#		-I$(NFDVR)/src/include/hi3536 \
#		-I$(NFDVR)/lib/include \
#		-I$(NFDVR)/gnulib/include \
#		-I$(NFDVR)/gnulib/include/sms \
#		-I$(NFDVR)/lib/nmf/nmf_include \
#		-I$(NFDVR)/lib/onvif/include/gsoap-onvif-2.8 \
#                -I$(NFDVR)/include
##		-I$(NFDVR)/lib/nmf/include

INCDIR = -I$(NFDVR)/src/include \
		-I$(NFDVR)/src/include/mtd-utils \
		-I$(NFDVR)/lib/include \
		-I$(NFDVR)/lib/onvif/include/gsoap-onvif-2.8 \
		-I$(NFDVR)/gnulib/include \
		-I$(NFDVR)/gnulib/include/sms \
		-I$(NFDVR)/include \
		-I$(NFDVR)/lib/gobj/include \
		-I$(NFDVR)/include/novatek \
		-I$(NFDVR)/src/service/mrtp \
		-I$(NFDVR)/src/service/ivcam

# include for filesys ( library )
INCDIR += -I$(TARGET_ROOT)/usr/include \
		-I$(TARGET_ROOT)/usr/local/include \
		-I$(TARGET_ROOT)/usr/local/include/gstreamer-0.10 \
		-I$(TARGET_ROOT)/usr/local/include/glib-2.0 \
		-I$(TARGET_ROOT)/usr/local/include/nabto \
		-I$(TARGET_ROOT)/usr/local/lib/glib-2.0/include \
		-I$(TARGET_ROOT)/usr/include/atk-1.0 \
		-I$(TARGET_ROOT)/usr/include/gtk-2.0 \
		-I$(TARGET_ROOT)/usr/include/hangul-1.0 \
		-I$(TARGET_ROOT)/usr/lib/gtk-2.0/include \
		-I$(TARGET_ROOT)/usr/include/cairo \
		-I$(TARGET_ROOT)/usr/include/pango-1.0 \
		-I$(TARGET_ROOT)/usr/include/fontconfig \
		-I$(TARGET_ROOT)/usr/include/freetype2 \
		-I$(TARGET_ROOT)/usr/include/libpng12 \
		-I$(TARGET_ROOT)/usr/local/include/libxml2 \
		-I$(TARGET_ROOT)/usr/include/neon \
		-I$(TARGET_ROOT)/usr/include/openssl

# kernel driver include
#INCDIR += -I$(KERNEL_PATH)/include -I$(KERNEL_PATH)/arch/arm/include \
#			-I$(NFDVR)/driver/driver_hi3536/dspcomm \
#			-I$(NFDVR)/driver/driver_hi3536/fpga_rs485 \
#			-I$(NFDVR)/driver/driver_hi3536/watchdog \
#			-I$(NFDVR)/driver/driver_hi3536/tlv320aic23 \
#			-I$(NFDVR)/driver/driver_hi3536/pannel \
#			-I$(NFDVR)/driver/driver_hi3536/tps2384 \
#			-I$(NFDVR)/driver/driver_hi3536/pd69104b1 \
#			-I$(NFDVR)/driver/driver_hi3536/switch \
#			-I$(NFDVR)/driver/driver_hi3536/ip804 \
#			-I$(NFDVR)/driver/driver_hi3536/hid

INCDIR += -I$(KERNEL_PATH)/include \
			-I$(NFDVR)/driver/dspcomm \
			-I$(NFDVR)/driver/alarm_out \
			-I$(NFDVR)/driver/alarm_in \
			-I$(NFDVR)/driver/keypad \
			-I$(NFDVR)/driver/board_pp \
			-I$(NFDVR)/driver/jog \
			-I$(NFDVR)/driver/shuttle \
			-I$(NFDVR)/driver/fpga_rs485 \
			-I$(NFDVR)/driver/watchdog \
			-I$(NFDVR)/driver/edid \
			-I$(NFDVR)/driver/drv_ext \
			-I$(NFDVR)/driver/micom \
			-I$(NFDVR)/driver/switch \
			-I$(NFDVR)/driver/ip804 \
			-I$(NFDVR)/driver/decoder

# for library

LDFLAGS += -Wl,--export-dynamic -Wl,-rpath-link=$(TARGET_ROOT)/usr/local/lib/nabto

#LIBDIR = -L$(NFDVR)/lib \
#		-L$(NFDVR)/gnulib \
#		-L$(NFDVR)/src/lib \
#		-L$(TARGET_ROOT)/usr/local/lib \
#		-L$(TARGET_ROOT)/usr/local/lib/gstreamer-0.10 \
#		-L$(TARGET_ROOT)/usr/lib \
#		-L$(NFDVR)/lib/nmf/lib \
#		-L$(NFDVR)/lib/hi35XX

#LIBS = -ldl -lz -lm -lpthread -lpthread-stubs \
#		-lesmtp -lssl -lcrypto -lneon -lgnutls -lgcrypt -lgpg-error  \
#		-lglib-2.0 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 \
#		-lgstreamer-0.10 -lgstbase-0.10 -lgstinterfaces-0.10 -lgstnet-0.10 \
#		-lfreetype -lfontconfig -lpango-1.0 -lpangoft2-1.0 -lcairo -lpangocairo-1.0  \
#		-latk-1.0 -lgtk-x11-2.0 -lgdk-x11-2.0 -lgdk_pixbuf-2.0 -lpixman-1 -lpng12 \
#		-lXrandr -lXext -lXcursor -lXcomposite -lXdamage -lXfixes -lXrender -lX11 -lxcb -lXau -lXdmcp \
#		-lexpat -lxml2 -lgio-2.0 -luuid -lXft -lpangoxft-1.0 -lSM -lICE \
#		-lgsoapssl -lonvif -ljansson -lrt -lcurl -lzmq -lsqlite3
		
#LIBS += -ljpeg

#LIBS +=	-litxupnp -lgsocket -ljansson -lunp -lproxy_cli -lums\
#		-lsst -larch -licmem \
#		-lnfdal -lgstnfbuddy  -lgstnflistbuffer -lgstnfapp -lgstmrtpsrc -lgstnfsstsrc -lipx_dispmux -lnmf -lnfedma -licalib -lissm \
#		-lhi_cipher -lmali -lOpenCL -lstdc++ -litxdnn_hi3536 -lEGL -lGLESv2 -lIMV1 -liwimv -lstdc++

LIBDIR = -L$(NFDVR)/lib \
		-L$(NFDVR)/gnulib \
		-L$(NFDVR)/src/lib \
		-L$(NFDVR)/lib/gobj/lib \
		-L$(TARGET_ROOT)/usr/local/lib \
		-L$(TARGET_ROOT)/usr/local/lib/nabto \
		-L$(TARGET_ROOT)/usr/local/lib64 \
		-L$(TARGET_ROOT)/usr/lib \
		-L$(TARGET_ROOT)/lib 

LIBS = -ldl -lz -lm -lpthread -lpthread-stubs \
		-lesmtp -lssl -lcrypto -lneon -lgnutls -lgcrypt -lgpg-error  \
		-lglib-2.0 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 \
		-lfreetype -lfontconfig -lpango-1.0 -lpangoft2-1.0 -lcairo -lpangocairo-1.0  \
		-latk-1.0 -lgtk-x11-2.0 -lgdk-x11-2.0 -lgdk_pixbuf-2.0 -lpixman-1 -lpng12 \
		-lXrandr -lXext -lXcursor -lXcomposite -lXdamage -lXfixes -lXrender -lX11 -lxcb -lXau -lXdmcp \
		-lexpat -lxml2 -lgio-2.0 -luuid -lXft -lpangoxft-1.0 -lSM -lICE -lcurl -lzmq -lsqlite3 -lrt \
		-lgsoapssl -lonvif -lnettle -lp11-kit -lgmp -lhogweed \
		-lhdal -lvendor_media -lprebuilt_md -lvendor_pcie -lvendor_cv -lvendor_isp \
		-lgobjbuddybuffer -lgobjlistbuffer -lgobjmedia \
  	 	-lpcre -lffi
#UTM4G		
LIBS += -ljpeg

LIBS +=	-lproxy_cli -litxupnp -lgsocket -ljansson -lunp -lums\
		-lsst -larch -licmem \
		-lnfdal -lipx_dispmux \
		-lissm -licalib -lzmq

# ── Nabto Edge SDK libs ─────────────────────────────
LIBS += -lnabto_device -lnm_iam

MACRO	+= $(RULES_MACRO)
CFLAGS	+= $(RULES_CFLAGS)
LDFLAGS	+= $(RULES_LDFLAGS)
LIBS	+= $(RULES_LIBS)
LIBDIR	+= $(RULES_LIBDIR)
INCDIR	+= $(RULES_INCDIR)

OBJS_SYSMAN = sysman/dlist.o \
		sysman/nf_display.o \
		sysman/nf_object.o \
		sysman/nf_sysdb.o \
		sysman/nf_watchdog.o \
		sysman/nf_notify.o \
		sysman/nf_event.o \
		sysman/nf_dva_event.o \
		sysman/nf_action.o \
		sysman/nf_timer.o \
		sysman/nf_debug.o \
		sysman/nf_dvrlib.o \
		sysman/nf_hw.o \
		sysman/nf_ptz.o \
		sysman/nf_ptz_proto_pelcoD.o \
		sysman/nf_ptz_proto_pelcoP.o \
		sysman/nf_ptz_proto_Mesa_Pelco.o \
		sysman/nf_ptz_proto_D_Max.o \
		sysman/nf_ptz_proto_fastraxII_2.o \
		sysman/nf_ptz_proto_Ganz_PT_V3_2.o \
		sysman/nf_ptz_proto_Scc641.o \
		sysman/nf_ptz_proto_Scc643a.o \
		sysman/nf_ptz_proto_Ptc200.o \
		sysman/nf_ptz_proto_Ptc400c.o \
		sysman/nf_ptz_proto_Spd3300.o \
		sysman/nf_ptz_proto_Takex.o \
		sysman/nf_ptz_proto_WVcs850.o \
		sysman/nf_ptz_proto_WVcsr604.o \
		sysman/nf_ptz_proto_Multix.o \
		sysman/nf_ptz_proto_Mrx1000.o \
		sysman/nf_ptz_proto_LPT_A100L.o \
		sysman/nf_ptz_proto_interM.o \
		sysman/nf_ptz_proto_Vitek.o \
		sysman/nf_ptz_proto_Lilin.o \
		sysman/nf_keyctrl.o \
		sysman/nf_keyctrl_proto_d_max.o \
		sysman/nf_keyctrl_proto_ganz.o \
		sysman/nf_keyctrl_proto_siemens.o \
		sysman/nf_keyctrl_proto_mesa.o \
		sysman/nf_keyctrl_proto_wonwoo.o \
		sysman/nf_keyctrl_proto_vicon.o \
		sysman/nf_keyctrl_proto_grundig.o \
		sysman/nf_util_netif.o \
		sysman/nf_util_wpa.o \
		sysman/nf_util_function.o \
		sysman/nf_util_snmp.o \
		sysman/nf_util_time.o \
		sysman/nf_util_device.o \
		sysman/nf_util_flash.o \
		sysman/nf_util_flash_nand_write.o \
		sysman/nf_util_flash_nand_read.o \
		sysman/nf_util_fw.o \
		sysman/nf_util_fw_hdd.o \
		sysman/nf_util_fw_hotkey.o \
		sysman/nf_util_fw_utils.o \
		sysman/nf_util_fw_single.o \
		sysman/nf_util_fw_crc.o \
		sysman/nf_util_fw_httpfs.o \
		sysman/nf_util_fw_s1.o \
		sysman/nf_util_mail.o \
		sysman/nf_util_ftp.o \
		sysman/nf_util_http.o \
		sysman/nf_util_sms.o \
		sysman/nf_util_push.o \
		sysman/nf_util_ntp.o \
		sysman/nf_util_jpeg.o \
		sysman/nf_util_sntp.o \
		sysman/nf_util_parse_s1.o \
		sysman/nf_sysman.o \
		sysman/nf_micom_upgrade.o \
		sysman/nf_auth.o \
		sysman/nf_session.o \
		sysman/jbshell.o \
		sysman/jbshell_cmd_sst.o \
		sysman/jbshell_cmd_dsp.o \
		sysman/jbshell_cmd_host.o \
		sysman/nf_pos.o \
		sysman/nf_pos_proto_ave.o \
		sysman/nf_guard.o \
		sysman/nf_ddc.o \
		sysman/nf_remote_assist_usb.o \
		sysman/nf_admintool.o \
		sysman/nf_hid.o \
		sysman/nf_edid.o \
		sysman/nf_edid_parse.o \
		sysman/nf_edid.itx.o \
		sysman/nf_edid.nvt.o \
		sysman/nf_qc_app.o \
		sysman/nf_qc_audio.o \
		sysman/nf_qc_main.o
#		sysman/nf_HI_aud_api.o \
#		sysman/nf_HI_common.o \
#		sysman/nf_HI_vo.o
#		sysman/nf_HI_aud_hi3536.o \
#		sysman/nf_HI_aud_hi3536_qc.o \
#		sysman/nf_tw2880_disp.o \
#		sysman/nf_solo_rec.o \
#		sysman/nf_solo_enc.o \
#		sysman/nf_solo_dec.o \
#		sysman/nf_solo_aud.o \
#		sysman/nf_solo_capture.o \
#		sysman/nf_solo_jpeg.o
# ksi_test		sysman/SEED_KISA.o \

OBJS_SERVICE = service/nf_network.o \
		service/nf_netsvr_ddns.o \
		service/nf_netsvr_ddns_s1.o \
		service/nf_netsvr_upnp.o \
		service/nf_netsvr_reqcmd.o \
		service/nf_meta_data.o \
		service/nf_record.o \
		service/nf_audio.nt9833x.o \
		service/nf_audio_put_frame.o \
		service/nf_audio_put_sst.o \
		service/nf_audio_cntl_stream.o \
		service/nf_audio_convert.o \
		service/nf_audio_novatek.nt9833x.o\
		service/nf_audio_pb_local.o \
		service/nf_audio_ai.o \
		service/nf_api_cam.o \
		service/nf_api_disk.o \
		service/nf_api_raid.o \
		service/nf_api_eventlog.o \
		service/nf_api_live.o \
		service/nf_va_object_detector.o \
		service/nf_api_ipcam.o \
		service/nf_api_param_app.o \
		service/nf_api_param_hw.o \
		service/nf_api_param_hw_enc.o \
		service/nf_api_param_fw.o \
		service/nf_api_param_fwver.o \
		service/nf_ipcam_unittest.o \
		service/nf_ipcam_setter.o \
		service/nf_api_openmode.o \
		service/nf_ipcam_pnd.o \
		service/nf_ipcam_models.o \
		service/nf_ipcam_discovery.o \
		service/nf_ipcam_discovery_onvif.o \
		service/nf_ipcam_discovery_dhcpd.o \
		service/nf_ipcam_setup.o \
		service/nf_ipcam_hub.o \
		service/nf_ipcam_driver_itx.o \
		service/nf_ipcam_driver_itx_openmode.o \
		service/nf_ipcam_driver_itx_ptz.o \
		service/nf_ipcam_driver_axis.o \
		service/nf_ipcam_driver_sony.o \
		service/nf_ipcam_driver_vivotek.o \
		service/nf_ipcam_driver_grundig.o \
		service/nf_ipcam_driver_messoa.o \
		service/nf_ipcam_driver_s1.o \
		service/nf_ipcam_driver_xiongmai.o \
		service/nf_ipcam_driver_huviron.o \
		service/nf_ipcam_driver_hdpro.o \
		service/nf_ipcam_utils.o \
		service/nf_nvs_driver_itx.o \
		service/nf_onvif_ns.o \
		service/nf_onvif_device.o \
		service/nf_onvif_deviceio.o \
		service/nf_onvif_media.o \
		service/nf_onvif_media2.o \
		service/nf_onvif_imaging.o \
		service/nf_onvif_ptz.o \
		service/nf_onvif_event.o \
		service/nf_onvif_analytics.o \
		service/nf_onvif_metadata_parser.o \
		service/axis_discovery.o \
		service/nf_api_play.o \
		service/nf_api_archive.o \
		service/nf_api_dva_eventlog.o \
		service/nf_api_pos_eventlog.o \
		service/nf_dspcomm_app.o \
		service/nf_va_statistic.o \
		service/ddns2_manager.o \
		service/ddns2_md5.o \
		service/s1_dual.o \
		service/nf_issm_ctl.o \
		service/nf_issm_ctl_bitrate.o \
		service/nf_issm_ctl_frame.o \
		service/nf_issm_ctl_funcs.o \
		service/nf_issm_ctl_live.o \
		service/nf_issm_ctl_playback.o \
		service/nf_issm_ctl_recvaudio.o	\
		service/nf_axis_imagesource_parser.o \
		service/nf_ipcam_capabilities_parser.o \
		service/nf_pds_ctl.o \
		service/nf_rtsp_over_ws.o \
		service/nf_net_console.o \
		service/nf_api_dlva.o \
		service/nf_api_http.o \
		service/nf_ipcam_zmq_utils.o \
		service/nf_ipcam_json_utils.o \
		service/nf_ipcam_vabox_data_operator.o \
		service/nf_ipcam_sql_utils.o \
		service/nf_packet_dump.o \
		service/curl_manager.o \
		service/admin_server_api.o \
		service/mrtp/gobjmrtpsrc.o \
		service/mrtp/gstmrtpclient.o \
		service/mrtp/gstmrtpvideo.o \
		service/mrtp/gstmrtpaudio.o \
		service/mrtp/gstmrtpmd5.o \
		service/mrtp/gstmrtpmeta.o \
		service/mrtp/SEED_KISA.o \
		service/mrtp/gobjmrtppipe.o \
		service/nabto/nabto_ctl.o \
		service/nabto/random_string.o \
		service/nf_encode.o \
		service/ivcam/ivcam_vca.o \
		service/ivcam/ivcam_rule.o \
		service/ivcam/ivcam_tracker.o
		
# wiggls : memory leak debug.
# mem_debug.o
	
		# ksi_test service/nf_rec_audio.o \
		#service/nf_rec_audio_ai.o \
#ksi_test 복원 작업 해야 함.
#		service/nf_cntl_audio.o \
#		service/nf_audio.nt9833x.o \

OBJS_EXTENSION = \
                        extension/nvs_service/nvs_discovery.o \
                        extension/nvs_service/nvs_onvif_event.o \
                        extension/nvs_service/nvs_onvif_app.o \
                        extension/nvs_service/nvs_onvif_app_util.o \
                        extension/cross_host/nvs_onvif_app_recording.o \
			extension/cross_host/onvif_cross_util.o \
                        extension/cross_host/nvs_onvif_app_event.o \
                        extension/nf_onvif_server.o \
                        extension/ezxml/ezxml.o

OBJS_LIBMTD = \
		../lib/libmtd/libmtd_legacy.o ../lib/libmtd/libmtd.o

include nfgui/Makefile.inc
include ../webra1/host/Makefile.inc
include ../webra/host/Makefile.inc

-include make.env.local

OBJS = $(OBJS_SYSMAN) $(OBJS_SERVICE) $(OBJS_GUI) $(OBJS_WEBRA) $(OBJS_WEBRA_API) nf_main.o $(OBJS_EXTENSION) $(OBJS_LIBMTD)
OBJS_SIMPLE = $(OBJS_SYSMAN) $(OBJS_SERVICE) nf_simple_main.o $(OBJS_EXTENSION) $(OBJS_LIBMTD)

SRCS := $(OBJS:.o=.c)

MOD_LIST := $(shell svn st | grep "^[MCA]" | awk '{print $2}')
ERROR_COLOR := 1;31
WARN_COLOR := 1;35

.c.o:
#	@echo "================================================================================"
	@echo " Compiling.....	[$<]"
#	@echo "================================================================================"
	@if [ ! -d warning/src/$(dir ${<}) ]; then mkdir -p warning/src/$(dir ${<}); fi
	@$(Q)$(CC) -o $(@D)/$(@F) $(CFLAGS) $(INCDIR) $(MACRO) -c $(@D)/$(<F) 2> warning/src/${<}.warning.tmp ; \
	if [ $$? = 1 ]; then export GREP_COLOR='$(ERROR_COLOR)'; cat warning/src/${<}.warning.tmp | grep --color ": error:\|: fatal error:"; exit 1; fi; \
	OUT_PUT=0; echo ${<} | grep -q "/webra/\|/webra1/\|/webra2/"; if [ $$? = 0 ]; then OUT_PUT=1; fi; \
	for i in ${MOD_LIST}; do if [ $$i = ${<} ]; then OUT_PUT=1; break; fi; done; \
	if [ $${OUT_PUT} = 1 ]; then export GREP_COLOR='$(WARN_COLOR)'; cat warning/src/${<}.warning.tmp \
	| grep -Ev "$(CROSS_COMPILE)|/usr/|/gnulib/|/ipx_kernel_ea13/" | grep --color ": warning:\|: note:"; fi; \
	rm -f warning/src/${<}.warning.tmp

#all :	$(TARGET) $(TARGET_SIMPLE)
all :	$(TARGET) $(TARGET_WEBRA)

$(TARGET) : $(OBJS)
	@echo ""
	@echo ""
	@echo "================================================================================"
	@echo " Linking.......	[$@] $(LIBDIR) $(LIBS) $(LDFLAGS)"
	@echo "================================================================================"
	@echo ""
	$(Q)$(CC) -o $(TARGET) $(LIBDIR) $(OBJS) $(LIBS) $(LDFLAGS)
	#$(STRIP) $(TARGET)
	chmod 755 $(TARGET)
	cp nf_host ../

$(TARGET_SIMPLE): $(OBJS_SIMPLE)
	@echo ""
	@echo ""
	@echo "================================================================================"
	@echo " Linking.......	[$@] $(LIBDIR) $(LIBS) $(LDFLAGS)"
	@echo "================================================================================"
	@echo ""
	$(Q)$(CC) -o $(TARGET_SIMPLE) $(LIBDIR) $(OBJS_SIMPLE) $(LIBS) $(LDFLAGS)
	#$(STRIP) $(TARGET)
	chmod 755 $(TARGET_SIMPLE)
	cp  $(TARGET_SIMPLE) ../

$(TARGET_WEBRA): ../webra/mk.conf
	@echo ""
	@echo "================================================================================"
	@echo " Making WebRA `cat ../webra/mk.conf`"
	@echo "================================================================================"
	@echo ""
	make -C ../webra
	@touch $@

clean: clean-ui clean-host clean-webra

clean-ui:
	rm -f $(OBJS_GUI) $(TARGET) nf_main.o
	rm -f $(OBJS_GUI) $(TARGET) nf_simple_main.o

clean-host: clean-service clean-sysman

clean-service:
	rm -f $(OBJS_SERVICE) $(TARGET) nf_main.o

clean-sysman:
	rm -f $(OBJS_SYSMAN) $(TARGET) nf_main.o

clean-webra:
	rm -f $(OBJS_WEBRA) $(TARGET) $(TARGET_WEBRA) nf_main.o $(OBJS_EXTENSION)

parangi:
	cp /nfsroot/NFDVR/nf_host /otmroot_filesys/NFDVR
	cp /nfsroot/NFDVR/data/nf_sysdb.conf /otmroot_filesys/NFDVR/data/
	cp /nfsroot/NFDVR/src/lib/libnfdal.a /otmroot_filesys/NFDVR/lib

vala: sysman/nf_session.vala
	@echo ""
	@echo "================================================================================"
	@echo " Coverting.vala to c....	[$<]  $(patsubst %.vala,%.h,$<)"
	@echo "================================================================================"
	@echo ""
	$(Q)valac --pkg=posix $< -C -H include/$(patsubst %.vala,%.h,$<)

