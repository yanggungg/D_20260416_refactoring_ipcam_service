#!/bin/sh

## set user path
USER_ROOT=~/filesys_ipx
HOST_ROOT=$USER_ROOT/NFDVR
export USER_ROOT

clear
echo "################################################################################"
echo "#                         start changing ui type                               #"
echo "################################################################################"
echo

echo "select gui type. "
echo "  1)ITX(100)    2)ITX(200) 	 3)ITX_B(DS)   4)ITX_C(DS)    5)SEATLE "
echo "  6)reserved    7)reserved     8)reserved    9)reserved    10)reserved "
echo " 11)CBC         12)XRPLUS      13)SAMSUNG    14)I3DVR(DS)   15)MACE(DS) "
echo " 16)HONEYWELL   17)ORION(DS)   18)DIGIMERGE  19)STL(DS)     20)ABUS     "
echo " 21)KODO(DS)    22)NOVUS       23)S1         24)VIDECON     25)LOREX    "
echo " 26)VITEK(DS)   27)OTM(temp)"
read VENDOR_NO

case "$VENDOR_NO" in
        "1" )
		TARGET_PNG=data/gui/png/100
		TARGET_COLOR_CFG=data/gui/cfg/color.cfg.100
        ;;
        "2" )
		TARGET_PNG=data/gui/png/200
		TARGET_COLOR_CFG=data/gui/cfg/color.cfg.200
        ;;
        "3" )
        exit 1;						
        ;;
        "4" )
        exit 1;						
        ;;
        "5" )
        exit 1;						
        ;;
        "6" )
        exit 1;
        ;;
        "7" )
        exit 1;
        ;;
        "8" )
        exit 1;
        ;;
        "9" )
        exit 1;
        ;;
        "10" )
        exit 1;
        ;;
        "11" )
		TARGET_PNG=data/gui/png/32
		TARGET_COLOR_CFG=data/gui/cfg/color.cfg.32
        ;;
        "12" )
        exit 1;				
        ;;
        "13" )
        exit 1;				
        ;;
        "14" )
        exit 1;				
        ;;
        "15" )
        exit 1;				
        ;;
        "16" )
        exit 1;				
        ;;
        "17" )
        exit 1;				
        ;;
        "18" )
        exit 1;				
        ;;
        "19" )
        exit 1;				
        ;;
        "20" )
        exit 1;				
        ;;
        "21" )
        exit 1;				
        ;;
        "22" )
        exit 1;				
        ;;
        "23" )
        exit 1;				
        ;;
        "24" )
        exit 1;				
        ;;
        "25" )
        exit 1;				
        ;;
        "26" )
        exit 1;				
        ;;
        "27" )
        exit 1;		
        ;;
        * )
        exit 1;
        ;;
esac
echo

rm $HOST_ROOT/data/gui/bmp/OTM/4D1/*
cp -av $HOST_ROOT/$TARGET_PNG/* $HOST_ROOT/data/gui/bmp/OTM/4D1/
rm $HOST_ROOT/data/gui/color.cfg
cp -v $HOST_ROOT/$TARGET_COLOR_CFG $HOST_ROOT/data/gui/color.cfg

