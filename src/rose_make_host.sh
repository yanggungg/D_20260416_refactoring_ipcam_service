#!/bin/sh

################   FUNCTION LIST !!!   #####################################################
function error_msg_if_fail
{
	if [ $1 != 0 ];	then
		echo "### ERROR : $2"
		exit 1
	fi
}

function do_make_db_sh
{
	rm -rf ../data/nf_sysdb_*.conf
	rm -rf nf_sysdb.conf
	sh make_db.sh $1 $2 $3 $4
	xmllint nf_sysdb.conf > tmp
	error_msg_if_fail $? "Make DB fail."
	mv nf_sysdb.conf ../data/nf_sysdb.conf
	echo "TYPE=$1" > make_db.env
	echo "MODEL=$2" >> make_db.env
	echo "VENDOR=$3" >> make_db.env
	echo "SIGNAL_TYPE=$4" >> make_db.env	
}

function do_make_ver_sh
{
	rm -rf ../data/nf_sysdb_version.conf
	rm -rf nf_sysdb_version.conf
	sh make_swver.sh nf_sysdb_version.conf $1 $2
	error_msg_if_fail $? "Make  fail."
	mv nf_sysdb_version.conf ../data/nf_sysdb_version.conf	
}

function do_make_nf_dal
{
	make TARGET_MODEL=$1 TARGET_VENDOR=$2 TARGET_ROOT=$3
	error_msg_if_fail $? "fail to build DAL. bye."
	make TARGET_MODEL=$1 TARGET_VENDOR=$2 TARGET_ROOT=$3 install
}

function do_copy_image
{
	rm -rf $HOST_ROOT/data/gui/bmp/OTM/4D1
	mkdir $HOST_ROOT/data/gui/bmp/OTM/4D1
	cp -av $HOST_ROOT/$TARGET_PNG/* $HOST_ROOT/data/gui/bmp/OTM/4D1/
	cp -av $HOST_ROOT/$TARGET_RES/*.png $HOST_ROOT/data/gui/bmp/OTM/4D1

	if [ -e $HOST_ROOT/$TARGET_RES/guide ]
	then
		cp -av $HOST_ROOT/$TARGET_RES/guide/*.png $HOST_ROOT/data/gui/bmp/OTM/4D1
	fi

	if [ -e $HOST_ROOT/$TARGET_RES/logo_image ]
	then
		cp -av $HOST_ROOT/$TARGET_RES/logo_image/*.png $HOST_ROOT/data/gui/bmp/OTM/4D1
	fi	
}

function do_make_tmpenv
{
	echo model : val={"$1"} > $HOST_ROOT/data/oem_info/tmp.env
	echo channel : val={"$2"} >> $HOST_ROOT/data/oem_info/tmp.env
	echo vendor : val={"$3"} >> $HOST_ROOT/data/oem_info/tmp.env
	echo png_path : val={"$TARGET_PNG"} >> $HOST_ROOT/data/oem_info/tmp.env
	echo res_path : val={"$TARGET_RES"} >> $HOST_ROOT/data/oem_info/tmp.env
	echo colorcfg_path : val={"$TARGET_COLOR_CFG"} >> $HOST_ROOT/data/oem_info/tmp.env
}

function do_make_nf_host
{
	make clean
	make -j 4
	error_msg_if_fail $? "fail to build host. bye."
}

function do_clean_web_ra
{
  echo "### Cleaning WEBRA on $WEBRA_PATH"
  pushd $WEBRA_PATH
  ./bootstrap.sh
  make clean
  error_msg_if_fail $? "fail to build web ra. bye."
  popd
}


function target_vendor_32bpp
{
	TARGET_PNG=data/gui/png/100
	TARGET_RES=data/gui/res/100
	TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
	TARGET_VDEL=data/gui/res/100/640x360.h264
			
	case "$1" in
		"ZICOM" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/101
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
		;;
		"ITX_A" )
			TARGET_PNG=data/gui/png/200
			TARGET_RES=data/gui/res/200
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.200
		;;
		"ITX_46GUI" )
			TARGET_PNG=data/gui/png/46
			TARGET_RES=data/gui/res/1400
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46
		;;
		"CBC" )
			TARGET_PNG=data/gui/png/32
			TARGET_RES=data/gui/res/32
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.32
		;;
		"NOVUS" )
			TARGET_PNG=data/gui/png/46
			TARGET_RES=data/gui/res/46
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46
		;;
		"ASP" )
			TARGET_PNG=data/gui/png/108
			TARGET_RES=data/gui/res/108
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.108
		;;
		"LOGICOM" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/107
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
		;;
		"GPS" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/95
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
		;;
		"VICON" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/96
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
		;;
		"I3DVR" )
			TARGET_PNG=data/gui/png/31
			TARGET_RES=data/gui/res/31
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.31
		;;
		"KMW" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/100
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
		;;
		"SGD" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/100
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
		;;
		"QVS" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/93
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
		;;
		"HONEYWELL" )
			TARGET_PNG=data/gui/png/18
			TARGET_RES=data/gui/res/18
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.18
		;;
		"S1" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/30
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
			TARGET_VDEL=data/gui/res/30/640x360.h264
		;;
		"KOBI" )
			TARGET_PNG=data/gui/png/46
			TARGET_RES=data/gui/res/76
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46
		;;
		"SUNSTATE" )
			TARGET_RES=data/gui/res/193
		;;
		"AMK" )
		;;
		"DSS" )
			TARGET_RES=data/gui/res/45
		;;
		"TELETEC" )
			TARGET_RES=data/gui/res/34
		;;
		"VIDECON" )
			TARGET_PNG=data/gui/png/28
			TARGET_RES=data/gui/res/28
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.28
		;;
		"VIDECON_US" )
			TARGET_PNG=data/gui/png/28
			TARGET_RES=data/gui/res/28
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.28
		;;
		"S1_JAPAN" )
			TARGET_PNG=data/gui/png/100
			TARGET_RES=data/gui/res/30
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
			TARGET_VDEL=data/gui/res/30/640x360.h264
		;;
		"G4S" )
			TARGET_PNG=data/gui/png/183
			TARGET_RES=data/gui/res/183
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.183
		;;
		"ORION" )
			TARGET_PNG=data/gui/png/183
			TARGET_RES=data/gui/res/83
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.183
		;;
		"TAKENAKA" )
			TARGET_PNG=data/gui/png/108
			TARGET_RES=data/gui/res/65
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.108
		;;
		"IMENSAZAN" )
			TARGET_PNG=data/gui/png/100
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
		;;
		"AMBUSH" )
			TARGET_PNG=data/gui/png/46
			TARGET_RES=data/gui/res/176
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46
		;;
		"CBC_UT" )
			TARGET_PNG=data/gui/png/32
			TARGET_RES=data/gui/res/232
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.32
		;;
		"ITX_ICA" )
			TARGET_PNG=data/gui/png/99
			TARGET_RES=data/gui/res/99
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.99
		;;
		"DAYOU" )
			TARGET_PNG=data/gui/png/43
			TARGET_RES=data/gui/res/43
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46
		;;

	esac
}

function target_vendor_16bpp
{
	TARGET_PNG=data/gui/png/100_16BPP
	TARGET_RES=data/gui/res/100
	TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
	TARGET_VDEL=data/gui/res/100/640x360.h264
	TARGET_BP=data/bplayer/BackupPlayer_ITX.exe


	case "$1" in
		"ZICOM" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/101
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
		;;
		"ITX_A" | "ITX_A_PAL" )
			TARGET_PNG=data/gui/png/200_16BPP
			TARGET_RES=data/gui/res/200
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.200.16bpp
		;;
		"ITX_46GUI" )
			TARGET_PNG=data/gui/png/46_16BPP
			TARGET_RES=data/gui/res/1400
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46.16bpp
		;;
		"CBC" )
			TARGET_PNG=data/gui/png/32_16BPP
			TARGET_RES=data/gui/res/32
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.32.16bpp
		;;
		"NOVUS" )
			TARGET_PNG=data/gui/png/46_16BPP
			TARGET_RES=data/gui/res/46
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46.16bpp
		;;
		"ASP" )
			TARGET_PNG=data/gui/png/108
			TARGET_RES=data/gui/res/108
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.108.16bpp
		;;
		"LOGICOM" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/107
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
		;;
		"GPS" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/95
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
		;;
		"VICON" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/96
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
		;;
		"I3DVR" )
			TARGET_PNG=data/gui/png/31
			TARGET_RES=data/gui/res/31
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.31.16bpp
		;;
		"KMW" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/100
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
		;;
		"SGD" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/100
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
		;;
		"QVS" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/93
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
		;;
		"HONEYWELL" )
			TARGET_PNG=data/gui/png/18_16BPP
			TARGET_RES=data/gui/res/18
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.18.16bpp
		;;
		"S1" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/30
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
			TARGET_VDEL=data/gui/res/30/640x360.h264
		;;
		"KOBI" )
			TARGET_PNG=data/gui/png/46_16BPP
			TARGET_RES=data/gui/res/76
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46.16bpp
		;;
		"SUNSTATE" )
			TARGET_RES=data/gui/res/193
		;;
		"AMK" )
		;;
		"DSS" )
			TARGET_RES=data/gui/res/45
		;;
		"TELETEC" )
			TARGET_RES=data/gui/res/34
		;;
		"VIDECON" )
			TARGET_PNG=data/gui/png/28_16BPP
			TARGET_RES=data/gui/res/28
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.28.16bpp
		;;
		"VIDECON_US" )
			TARGET_PNG=data/gui/png/28_16BPP
			TARGET_RES=data/gui/res/28
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.28.16bpp
		;;
		"S1_JAPAN" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_RES=data/gui/res/30
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
			TARGET_VDEL=data/gui/res/30/640x360.h264
		;;
		"G4S" )
			TARGET_PNG=data/gui/png/183_16BPP
			TARGET_RES=data/gui/res/183
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.183.16bpp
		;;
		"ORION" )
			TARGET_PNG=data/gui/png/183_16BPP
			TARGET_RES=data/gui/res/83
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.183.16bpp
		;;
		"TAKENAKA" )
			TARGET_PNG=data/gui/png/108
			TARGET_RES=data/gui/res/65
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.108.16bpp
		;;
		"IMENSAZAN" )
			TARGET_PNG=data/gui/png/100_16BPP
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100.16bpp
		;;
		"AMBUSH" )
			TARGET_PNG=data/gui/png/46_16BPP
			TARGET_RES=data/gui/res/176
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.46.16bpp
		;;
		"CBC_UT" )
			TARGET_PNG=data/gui/png/32_16BPP
			TARGET_RES=data/gui/res/232
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.32.16bpp
		;;
		"ITX_ICA" )
			TARGET_PNG=data/gui/png/99_16BPP
			TARGET_RES=data/gui/res/99
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.99.16bpp
		;;
		"DAYOU" )
			TARGET_PNG=data/gui/png/43_16BPP
			TARGET_RES=data/gui/res/43
			TARGET_COLOR_CFG=data/gui/cfg/color.cfg.43.16bpp
		;;

	esac
}
############################################################################################


## debug echo prefix
DBG=" ### "
ERR=" ### ERROR :"

## to set custom user path, set variables in rose_make_conf.sh
USER_ROOT=$PWD/../..
HOST_ROOT=$USER_ROOT/NFDVR

echo $USER_ROOT
ROSE_CONF=rose_make_conf.sh
if [ -f $ROSE_CONF ] ; then
  . ./$ROSE_CONF
fi

export USER_ROOT
SIGNAL_TYPE=NTSC

echo "::::::::::::::::::::::::::::: MODEL TYPE: $1"
echo "::::::::::::::::::::::::::::: MODEL CH: $2"
echo "::::::::::::::::::::::::::::: VENDOR TYPE: $3"
echo "::::::::::::::::::::::::::::: MODEL TYPE: $4"

case "$3" in
  "itx_basic" )
  VENDOR=ITX
  ;;
	
  "itx_zicom" )
  VENDOR=ZICOM
  ;;

  "itx_a" )
  VENDOR=ITX_A
  ;;

  "itx_b" )
  VENDOR=ITX_B
  ;;

  "itx_c" )
  VENDOR=ITX_C
  ;;

  "itx_46gui" )
  VENDOR=ITX_46GUI
  ;;

  "itx_cbc" )
  VENDOR=CBC
  ;;

  "itx_xrplus" )
  VENDOR=XRPLUS
  ;;

  "itx_i3dvr" )
  VENDOR=I3DVR
  ;;

  "itx_mace" )
  VENDOR=MACE
  ;;

  "itx_honeywell" )
  VENDOR=HONEYWELL
  ;;

  "itx_g4s" )
  VENDOR=G4S
  SIGNAL_TYPE=PAL
  ;;

  "itx_lorex" )
  VENDOR=LOREX
  ;;

  "itx_seatle" )
  VENDOR=SEATLE
  ;;

  "itx_sme2220" )
  VENDOR=SAMSUNG
  ;;

  "itx_dimg" )
  VENDOR=DIGIMERGE
  ;;

  "itx_stl" )
  VENDOR=STL
  ;;

  "itx_abus" )
  VENDOR=ABUS
  ;;

  "itx_kodo" )
  VENDOR=KODO
  ;;

  "itx_dayou" )
  VENDOR=DAYOU
  ;;
  
  "itx_vitek" )
  VENDOR=VITEK
  ;;
  
  "itx_otm" )
  VENDOR=OTM
  ;;

  "itx_logicom" )
  VENDOR=LOGICOM
  SIGNAL_TYPE=PAL
  ;;

  "itx_asp" )
  VENDOR=ASP
  SIGNAL_TYPE=PAL
  ;;

  "itx_gps" )
  VENDOR=GPS
  SIGNAL_TYPE=PAL
  ;;

  "itx_vicon" )
  VENDOR=VICON
  SIGNAL_TYPE=PAL
  ;;

  "itx_kmw" )
  VENDOR=KMW
  ;;

  "itx_sgd" )
  VENDOR=SGD
  SIGNAL_TYPE=PAL
  ;;

  "itx_qvs" )
  VENDOR=QVS
  SIGNAL_TYPE=PAL
  ;;

  "itx_s1" )
  VENDOR=S1
  ;;

  "S1_pal" )
  VENDOR=S1
  SIGNAL_TYPE=PAL
  ;;

  "itx_kobi" )
  VENDOR=KOBI
  SIGNAL_TYPE=PAL
  ;;

  "itx_sunstate" )
  VENDOR=SUNSTATE
  SIGNAL_TYPE=PAL
  ;;

  "itx_amk" )
  VENDOR=AMK
  ;;

  "itx_dss" )
  VENDOR=DSS
  ;;

  "itx_teletec" )
  VENDOR=TELETEC
  SIGNAL_TYPE=PAL
  ;;

  "itx_ipcam_zig" )
  VENDOR=IPCAM_ZIG
  ;;

  "itx_videcon" )
  VENDOR=VIDECON
  SIGNAL_TYPE=PAL
  ;;

  "itx_videcon_us" )
  VENDOR=VIDECON_US
  ;;

  "itx_s1_japan" )
  VENDOR=S1_JAPAN
  ;;

  "itx_orion" )
  VENDOR=ORION
  SIGNAL_TYPE=PAL
  ;;

  "itx_takenaka" )
  VENDOR=TAKENAKA
  ;;

  "itx_imensazan" )
  VENDOR=IMENSAZAN
  SIGNAL_TYPE=PAL
  ;;

  "itx_ambush" )
  VENDOR=AMBUSH
  SIGNAL_TYPE=PAL
  ;;

  "itx_cbc_ut" )
  VENDOR=CBC_UT
  SIGNAL_TYPE=PAL
  ;;

  "itx_irlab" )
  VENDOR=IRLAB
  ;;  
  
  "itx_s1_ob" )
  VENDOR=S1_OB
  ;;

  "itx_ica" )
  VENDOR=ITX_ICA
  ;;

  * )
  error_msg_if_fail 1 "invalid or not implemented vendor type."
  ;;
esac

OPTION=$5
NABTO=$6

#############################################################################
echo $DBG "clean up and make DB files..."

do_make_db_sh $1 $2 $VENDOR $SIGNAL_TYPE
do_make_ver_sh $VENDOR $OPTION
#############################################################################

#############################################################################
echo $DBG "make nf_multilang_string_utf8_new.txt files..."

do_make_lang_sh
#############################################################################

if [ "$1" = "IPXP5" ]
then
	#########################################################################
	## STANDALONE
	if [ "$4" = "std" ]
	then
		TARGET_WEBRA=webra-ipx

		case "$2" in
			#################################################################
			## IPXP5/STD/32CH
			"32" )
				MODEL=_IPX_32P5
				GBOJ_MODEL_TYPE=IPXP5
				TARGET_MAKEFILE=Makefile.nvt.ipxp5
				cp -r $HOST_ROOT/lib/sqlite3/8g/lib/*sql* $USER_ROOT/usr/local/lib/
				cp -r $HOST_ROOT/lib/sqlite3/8g/include/*sql* $USER_ROOT/usr/local/include/

				TARGET_UX_CONF=data/gui/cfg/uxconf.ipxve3
				target_vendor_16bpp $VENDOR
				cp $HOST_ROOT/onvif/lighttpd/htdocs/cgi-bin/onvif-main_32ch.fcgi $HOST_ROOT/onvif/lighttpd/htdocs/cgi-bin/onvif-main.fcgi
			;;
			## default
			"*" )
			echo $ERR"invalid model ch:"$2
			exit 1;
			;;
		esac
	else
		echo $ERR"invalid model type:"$4
		exit 1;
	fi
else
    echo $ERR"invalid model type:"$1
    exit 1;
fi

echo
echo $DBG "start building MODEL:"$MODEL/"VENDOR:"$VENDOR "..."
echo $DBG "!your host root path must be "$HOST_ROOT

echo "TARGET_MODEL=$MODEL" > make.env
echo "TARGET_MODEL=$MODEL" > ../make.env.model
echo "TARGET_VENDOR=$VENDOR" >> make.env
echo "TARGET_VENDOR=$VENDOR" >> ../make.env.model
echo "TARGET_OPTION=$OPTION" >> make.env
echo "TARGET_NABTO=$NABTO" >> make.env
echo "TARGET_ROOT=$USER_ROOT" >> make.env
echo

if [ ! -f '../log/log_000.log' ]
then
  echo "### copy log ###"
  cp ../log/log_000_org.log ../log/log_000.log
else
  echo "### no need to copy log ###"
fi

#echo "##########################################################################"
#echo "## symbolic link module path..(TODO)"
#echo "##########################################################################"
#MODULE_PATH=../driver/anf/davinci
#if [ -d $MODULE_PATH ]
#    then
#        echo "your symbolic link for module:"$MODULE_PATH" will be deleted."
#        rm $MODULE_PATH
#fi
#ln -vs $TARGET_MOD_PATH $MODULE_PATH
#echo
#
#echo $DBG "set module..."
#cp -v ../driver/$TARGET_MOD ../driver/loadmodules.sh
#echo

echo "###########################################################################"
echo "## Prepare to Build of Novatek"
echo "###########################################################################"
GBOJ_ROOT=$HOST_ROOT/lib/gobj
GBOJ_SRC=$GBOJ_ROOT/$GBOJ_MODEL_TYPE
GBOJ_DST=$GBOJ_ROOT

pushd .
cd $HOST_ROOT/
cp -av run_ipx.sh.nvt.ipxp5 run_ipx.sh

rm driver
if [[ "$MODEL" = "_IPX_32P5" ]]
then
	ln -s driver.nt9833x.ipxp5 driver
	cd driver
	cd -

	cp -av ./data/install/ramdisk_fwup_256M.gz ./data/install/ramdisk_fwup.gz
else
	exit	
fi

if [[ "$MODEL" = "_IPX_32P5" ]]
then
	pushd .
	cd src/include
	rm novatek
	ln -s novatek.nt9833x novatek
	popd
else
	exit
fi

#cd $HOST_ROOT/src/include
#rm $MDL_TRG
#ln -s $MDL_SRC $MDL_TRG

#cd $HOST_ROOT/lib
#rm $MDL_TRG
#ln -s $MDL_SRC $MDL_TRG

cd $HOST_ROOT/lib/lib_sst
cp -vdpR libsst.so $HOST_ROOT/lib/
cp -vdpR libarch.so $HOST_ROOT/lib/
cp -vdpR libicmem.so $HOST_ROOT/lib/
cp -vdpR libnfedma.so $HOST_ROOT/lib/

cd $HOST_ROOT/lib/lib_sst
cp -vdpR libicalib.so $HOST_ROOT/lib/

cd $HOST_ROOT/lib/$MDL_SRC
cp -vdpR *.so $HOST_ROOT/lib/
popd

# for mtd-utils
pushd .
cd $HOST_ROOT/lib/libmtd
cp -av libmtd.o.back libmtd.o
cp -av libmtd_legacy.o.back libmtd_legacy.o
popd

echo "##########################################################################"
echo "## setup gobj & onvif "
echo "##########################################################################"

if ! [ -d $GBOJ_SRC/include ]; then
    echo $ERR"dir:"$GBOJ_SRC"/include is not exist!"
    exit 1
fi

if ! [ -d $GBOJ_SRC/lib ]; then
    echo $ERR"dir:"$GBOJ_SRC"/lib is not exist!"
    exit 1
fi

cd $HOST_ROOT/src 
sh ./local_lib_update.sh $USER_ROOT $GBOJ_SRC $GBOJ_DST

echo "############## $GBOJ_SRC/include/ ####################"
echo "##########################################################################"
echo "## setup webra path "
echo "##########################################################################"
WEBRA1_PATH=../webra1
WEBRA_PATH=../webra

if [ -n "$USE_CUSTOM_WEB" ] ; then
  TARGET_WEBRA=$USE_CUSTOM_WEB
  install -d $HOST_ROOT/$TARGET_WEBRA
fi

if [ -e $WEBRA_PATH ]
then
  echo "[WEBRA]your web ra path:"$WEBRA_PATH" will be deleted."
  rm $WEBRA_PATH
fi

#TODO webra2 must be fixed
ln -sf webra2 $WEBRA_PATH
echo "WEBRA_PATH=$WEBRA_PATH" >> make.env

## make symbolic link for webra to webra working copy
pushd include
find ../../webra/host/include/*.h -type f -exec ln -svf {} \;
popd
pushd sysman
find ../../webra/host/sysman/*.c -type f -exec ln -svf {} \;
popd


echo "##########################################################################"
echo "## Cleaning webra."
echo "##########################################################################"
echo "### Cleaning WEBRA1 on $WEBRA_PATH"
pushd $WEBRA1_PATH
./bootstrap.sh
make clean
popd
echo "### Cleaning WEBRA on $WEBRA_PATH"
pushd $WEBRA_PATH
./bootstrap.sh
make clean
popd

echo "##########################################################################"


################################################################################
## copy Makefile.Rules before starting release script
################################################################################
pushd .
cd $HOST_ROOT/src
cp -v ./Makefile.Rules.tpl ./Makefile.Rules
popd

echo "###########################################################################"
echo "## build & install DAL."
echo "###########################################################################"
cd $HOST_ROOT/src/nfdal

do_make_nf_dal $MODEL $VENDOR $USER_ROOT

echo "##########################################################################"
echo "## build host."
echo "##########################################################################"
cd $HOST_ROOT/src
cp -v ./$TARGET_MAKEFILE ./Makefile
do_copy_image
rm $HOST_ROOT/data/gui/color.cfg
cp -v $HOST_ROOT/$TARGET_COLOR_CFG $HOST_ROOT/data/gui/color.cfg
rm $HOST_ROOT/data/gui/uxconf.cfg
cp -v $HOST_ROOT/$TARGET_UX_CONF $HOST_ROOT/data/gui/uxconf.cfg
rm -rf $HOST_ROOT/data/gui/login_user.itx
cp -v $HOST_ROOT/$TARGET_BP $HOST_ROOT/data/bplayer/BackupPlayer.exe
do_make_tmpenv $1 $2 $VENDOR
do_make_nf_host

################################################################################
## write down water-mark.
################################################################################
rm ./*.built
touch ./$MODEL.$VENDOR.built

## print out build info.
echo
echo $DBG BUILD INFO. $DBG
echo MODEL:$MODEL/ VENDOR:$VENDOR/ ROOT FS:$USER_ROOT
echo
