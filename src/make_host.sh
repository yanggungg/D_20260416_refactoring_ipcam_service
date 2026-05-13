#!/bin/sh

clear

if [ ! $1 ]
then
echo "################################################################################"
echo "#                            start building host                               #"
echo "################################################################################"
echo
echo "1. Select target model. "
echo "   1) IPXP5 32CH "
#echo "	 2) IPXP5 16CH"
read MODEL_NO

echo "2. Select vendor type. "
echo "   1)ITX(100)         28)VIDECON          65)TAKENAKA" 
#echo "  18)HONEYWELL      55)VITEK       108)ASP      95)GPS      	98)Total Security"
#echo "  106)SGD           31)I3DVR       28)VIDECON   83)ORION   	32)CBC"
#echo "  76)KOBI           65)TAKENAKA    109)CSI      115)ASWAR   	293)QVS_RED"
#echo "  117)EKONI_MEXICO  48)DODWELL     20)CYTE      183)G4S     	96)VICON"
#echo "  78)KB_DEVICE      91)ITXM        43)DAYOU     101)SEQURINET	94)VT"
read VENDOR_NO

echo "3. select option - VA"
echo "1) NONE"
echo "2) VA"
read OPTION_NO

echo "4. select option - NABTO"
echo "1) NONE"
echo "2) NABTO"
read NABTO_NO

else
	if [ $1 == PACKAGING ]
	then
		echo $2
		echo $3
		echo $4
		case $2 in
			"IPXP5" )
				case $3 in
		        	"32" )
						MODEL_NO="1"
					;;
			        "16" )
						MODEL_NO="2"
					;;
					"08" )
						MODEL_NO="3"
					;;
					* )
				        echo $ERR"no model to build."
			        	exit 1;
					;;
				esac
			;;
			* )
				echo $ERR"no model to build."
				exit 1;
			;;
		esac

		echo $4

		case $5 in
    	    "100" )
				VENDOR_NO="1"
			;;
			"101" )
				VENDOR_NO="101"
			;;
			"1400" )
				VENDOR_NO="2"
			;;
    	    "18" )
				VENDOR_NO="18"
			;;
    	    "55" )
				VENDOR_NO="55"
			;;
			"108" )
				VENDOR_NO="108"
			;;	
			"95" )
				VENDOR_NO="95"
			;;	
			"98" )
				VENDOR_NO="98"
			;;
			"106" )
				VENDOR_NO="106"
			;;	
			"31" )
				VENDOR_NO="31"
			;;
		    "28" )
				VENDOR_NO="28"					
			;;
		    "83" )
				VENDOR_NO="83"					
			;;
		    "32" )
				VENDOR_NO="32"					
			;;
			"76" )
				VENDOR_NO="76"					
			;;
			"65" )
				VENDOR_NO="65"					
			;;
			"109")
				VENDOR_NO="109"
			;;
			"115")
				VENDOR_NO="115"
			;;
			"293")
				VENDOR_NO="293"
			;;
			"117")
				VENDOR_NO="117"
			;;
			"48")
				VENDOR_NO="48"
			;;
			"20")
				VENDOR_NO="20"
			;;
			"183")
				VENDOR_NO="183"
			;;
			"96")
				VENDOR_NO="96"
			;;
			"78")
				VENDOR_NO="78"
			;;
			"43")
				VENDOR_NO="43"
			;;
			"94")
				VENDOR_NO="94"
			;;
			* )
		        echo $ERR"invalid or not implemented vendor type."
		        exit 1;
	        ;;				
		esac

		case $6 in
			"0" )
				OPTION_NO="1"
			;;
			"1" )
				OPTION_NO="2"
			;;
			* )
		        echo $ERR"invalid or not implemented option type."
		        exit 1;
	        ;;				
		esac	
		
		case "$7" in
			"0" )
			NABTO_NO="1"
			;;
			"1" )
			NABTO_NO="2"
			;;
			* )
				echo $ERR"invalid or not implemented option type."
				exit 1;
				;;				
		esac	
	else
        echo $ERR"invalid mode."
        exit 1;
	fi
fi

case "$MODEL_NO" in
        #####################################################################
		# IPXP5
        "1" )
        MODEL_TYPE0=IPXP5
        MODEL_TYPE1=std
        MODEL_CH=32
        ;;
        "2" )
        MODEL_TYPE0=IPXP5
        MODEL_TYPE1=std
        MODEL_CH=16
        ;;
        * )
        echo $ERR"Invalid or not implemented model type!"
        exit 1;
        ;;
esac

case "$VENDOR_NO" in
        "1" )
        VENDOR_TYPE=itx_basic
        ;;
		"101" )
        VENDOR_TYPE=itx_sequrinet
        ;;
		"2" )
        VENDOR_TYPE=itx_46gui
        ;;
        "18" )
        VENDOR_TYPE=itx_honeywell
        ;;
        "55" )
        VENDOR_TYPE=itx_vitek
        ;;
        "108" )
        VENDOR_TYPE=itx_asp
        ;;
        "95" )
        VENDOR_TYPE=itx_gps
        ;;
        "98" )
        VENDOR_TYPE=itx_TotalSecurity
        ;;
        "106" )
        VENDOR_TYPE=itx_sgd
        ;;
        "31" )
        VENDOR_TYPE=itx_i3dvr
        ;;
        "28" )
        VENDOR_TYPE=itx_videcon
        ;;
        "83" )
        VENDOR_TYPE=itx_orion
        ;;
        "32" )
        VENDOR_TYPE=itx_cbc
        ;;
		"76" )
        VENDOR_TYPE=itx_kobi
        ;;
		"65" )
        VENDOR_TYPE=itx_takenaka
        ;;
		"109" )
        VENDOR_TYPE=itx_csi
        ;;
		"115" )
        VENDOR_TYPE=itx_aswar
        ;;
		"293" )
        VENDOR_TYPE=itx_qvs_red
        ;;
		"117" )
        VENDOR_TYPE=itx_ekoni_mexico
        ;;
		"48" )
        VENDOR_TYPE=itx_dodwell
        ;;
		"20" )
        VENDOR_TYPE=itx_cyte
        ;;
		"183" )
        VENDOR_TYPE=itx_g4s
        ;;
		"96" )
        VENDOR_TYPE=itx_vicon
        ;;
		"91" )
        VENDOR_TYPE=itx_itxm
        ;;
		"78" )
        VENDOR_TYPE=itx_kb_device
        ;;
		"43" )
		VENDOR_TYPE=itx_da_you
        ;;
		"94" )
		VENDOR_TYPE=itx_vt
        ;;
        * )
        echo $ERR"invalid or not implemented vendor type!"
        exit 1;
        ;;
esac
echo
case "$OPTION_NO" in
	"1" )
	OPTION_TYPE="NONE"
	;;
	"2" )
	OPTION_TYPE="VA"
	;;
	* )
	echo $ERR"invalid or not implemented option type."
	exit 1;
    	;;				
esac

case "$NABTO_NO" in
	"1" )
	NABTO_TYPE="NONE"
	;;
	"2" )
	NABTO_TYPE="NABTO"
	;;
	* )
	NABTO_TYPE="NONE"
	;;				
esac

echo "sh ./rose_make_host.sh $MODEL_TYPE0 $MODEL_CH $VENDOR_TYPE $MODEL_TYPE1 $OPTION_TYPE $NABTO_TYPE"
sh ./rose_make_host.sh $MODEL_TYPE0 $MODEL_CH $VENDOR_TYPE $MODEL_TYPE1 $OPTION_TYPE $NABTO_TYPE
