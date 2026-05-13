#!/bin/sh

# debug prefix
DBG_PRE="install>> "

############################################################################
# 0-0. check input path
echo $DBG_PRE "start to install gobj & onvif."
if [ "$1" == "" ]
then
    echo $DBG_PRE ERROR! no destination path
    echo ""
    exit 1;
fi

if [ "$2" == "" ]
then
    echo $DBG_PRE ERROR! no destination path
    echo ""
    exit 1;
fi

if [ "$3" == "" ]
then
    echo $DBG_PRE ERROR! no destination path
    echo ""
    exit 1;
fi

USER_ROOT=$1
ONVIF_SRC=$USER_ROOT/NFDVR/lib/onvif
ONVIF_DST=$USER_ROOT/usr

GBOJ_SRC=$2
GBOJ_DST=$3

############################################################################
# 0-1. start update lib(s).
echo
echo "update usr/local/lib"
cp -vfa $ONVIF_SRC/lib/*.so*                                $ONVIF_DST/local/lib/
cp -vf $ONVIF_SRC/include/gsoap-onvif-2.8/*.nsmap           $ONVIF_DST/local/include/gsoap-onvif-2.8/
cp -vf $ONVIF_SRC/include/gsoap-onvif-2.8/*.h               $ONVIF_DST/local/include/gsoap-onvif-2.8/

cp -vdpR $GBOJ_SRC/lib/*.so*                    	    $GBOJ_DST/lib/
cp -vdpR $GBOJ_SRC/lib/hdal/*.so*                    	    $USER_ROOT/usr/lib
cp -vd $GBOJ_SRC/include/*                                  $GBOJ_DST/include

echo "done!!"
echo "done!!"
