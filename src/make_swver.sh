#!/bin/sh

FILENAME_VER_CONF=$1
VENDOR=$2
OPTION1=$3
VER_FORMAT=18310.1.1

if [ $VENDOR = "ITX" ]; then
	VENDOR_NO=100	
elif [ $VENDOR = "ZICOM" ]; then
	VENDOR_NO=101
elif [ $VENDOR = "ITX_A" ]; then
	VENDOR_NO=200
elif [ $VENDOR = "ITX_46GUI" ]; then
	VENDOR_NO=1400
elif [ $VENDOR = "IMENSAZAN" ]; then
	VENDOR_NO=116	
elif [ $VENDOR = "HONEYWELL" ]; then
	VENDOR_NO=18
elif [ $VENDOR = "VIDECON" ]; then
	VENDOR_NO=28
elif [ $VENDOR = "VIDECON_US" ]; then
	VENDOR_NO=128
elif [ $VENDOR = "S1" ]; then
	VENDOR_NO=30
elif [ $VENDOR = "S1_JAPAN" ]; then
	VENDOR_NO=130		
elif [ $VENDOR = "I3DVR" ]; then
	VENDOR_NO=31
elif [ $VENDOR = "CBC" ]; then
	VENDOR_NO=32
elif [ $VENDOR = "TELETEC" ]; then
	VENDOR_NO=34	
elif [ $VENDOR = "DAYOU" ]; then
	VENDOR_NO=43
elif [ $VENDOR = "DSS" ]; then
	VENDOR_NO=45
elif [ $VENDOR = "NOVUS" ]; then
	VENDOR_NO=46
elif [ $VENDOR = "TAKENAKA" ]; then
	VENDOR_NO=65
elif [ $VENDOR = "KOBI" ]; then
	VENDOR_NO=76
elif [ $VENDOR = "AMBUSH" ]; then
	VENDOR_NO=176	
elif [ $VENDOR = "ORION" ]; then
	VENDOR_NO=83	
elif [ $VENDOR = "G4S" ]; then
	VENDOR_NO=183	
elif [ $VENDOR = "GPS" ]; then
	VENDOR_NO=95
elif [ $VENDOR = "QVS" ]; then
	VENDOR_NO=93
elif [ $VENDOR = "SUNSTATE" ]; then
	VENDOR_NO=193
elif [ $VENDOR = "VICON" ]; then
	VENDOR_NO=96
elif [ $VENDOR = "AMK" ]; then
	VENDOR_NO=98	
elif [ $VENDOR = "KMW" ]; then
	VENDOR_NO=104	
elif [ $VENDOR = "LOGICOM" ]; then
	VENDOR_NO=107	
elif [ $VENDOR = "ASP" ]; then
	VENDOR_NO=108	
elif [ $VENDOR = "SGD" ]; then
	VENDOR_NO=106
elif [ $VENDOR = "CBC_UT" ]; then
	VENDOR_NO=232
elif [ $VENDOR = "S1_OB" ]; then
	VENDOR_NO=230
elif [ $VENDOR = "ITX_ICA" ]; then
        VENDOR_NO=99	
elif [ $VENDOR = "IRLAB" ]; then
        VENDOR_NO=199			
elif [ $VENDOR = "IPCAM_ZIG" ]; then
	VENDOR_NO=999	
else 
    echo $ERR"invalid vendor type:"$VENDOR
    exit 1;
fi

if [ $OPTION1 = "NONE" ]; then
	OPTION1_TEXT=""
elif [ $OPTION1 = "VA" ]; then
	OPTION1_TEXT="A"
elif [ $OPTION1 = "DEBUGGING" ]; then
	OPTION1_TEXT="A"
else 
    echo $ERR"invalid option type:"$OPTION1
    exit 1;
fi

FW_PARAM_VER_NAME=$VER_FORMAT.$VENDOR_NO$OPTION1_TEXT

echo "<nf_sysdb>"  	> $FILENAME_VER_CONF
echo "<sys>"      	>> $FILENAME_VER_CONF
echo "<item key=\"sys.info.swver\"	type=\"STRING\"   min=\"0\" max=\"64\" val=\"$FW_PARAM_VER_NAME\" />"\		>> $FILENAME_VER_CONF
echo "</sys>"		>> $FILENAME_VER_CONF
echo "</nf_sysdb>"	>> $FILENAME_VER_CONF
  
