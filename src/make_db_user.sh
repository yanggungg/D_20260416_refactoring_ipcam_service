#!/bin/bash
TYPE=$1
MODEL=$2
VENDOR=$3


# Common Value #
USR_GROUP1_AUDIO='<item key="usr.grp.G1.audio"			type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP1_MICROPHONE='<item key="usr.grp.G1.microphone"		type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP1_ARCHIVE='<item key="usr.grp.G1.archive"			type="BOOL" 	min="0" max="1" val="0" />'
USR_GROUP1_SYS_SETUP='<item key="usr.grp.G1.sys_setup"		type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP1_REC_SETUP='<item key="usr.grp.G1.rec_setup"		type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP1_REMOTE='<item key="usr.grp.G1.remote"			type="BOOL" 	min="0" max="1" val="0" />'
USR_GROUP1_SHUTDOWN='<item key="usr.grp.G1.shutdown"			type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP2_SEARCH='<item key="usr.grp.G2.search"			type="BOOL" 	min="0" max="1" val="0" />'
USR_GROUP2_EVENT='<item key="usr.grp.G2.event"			type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP2_AUDIO='<item key="usr.grp.G2.audio"			type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP2_MICROPHONE='<item key="usr.grp.G2.microphone"		type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP2_REMOTE='<item key="usr.grp.G2.remote"			type="BOOL" 	min="0" max="1" val="0" />'
USR_GROUP2_SHUTDOWN='<item key="usr.grp.G2.shutdown"			type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP0_SEQURINET='<item key="usr.grp.G0.sequrinet"			type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP1_SEQURINET='<item key="usr.grp.G1.sequrinet"			type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP2_SEQURINET='<item key="usr.grp.G2.sequrinet"			type="BOOL" 	min="0" max="1" val="1" />'
USR_GROUP3_SEQURINET='<item key="usr.grp.G3.sequrinet"			type="BOOL" 	min="0" max="1" val="0" />'
USR_GROUP4_SEQURINET='<item key="usr.grp.G4.sequrinet"			type="BOOL" 	min="0" max="1" val="0" />'

USR_GRP_G3_AUDIO='<item key="usr.grp.G3.audio"			type="BOOL" 	min="0" max="1" val="0" />'
USR_GRP_G3_MICROPHONE='<item key="usr.grp.G3.microphone"		type="BOOL" 	min="0" max="1" val="0" />'

USR_INIT_AUTOLOGOUT='<item key="usr.auto_logout"				type="BOOL"		min="0" max="1" val="0" />'
USR_INIT_AUTOLOGOUT_MIN='<item key="usr.auto_logout_min"			type="UINT"		min="0" max="999" val="1" />'

# Individual value(MODEL) first#
case "$TYPE" in
	"IPXM4B")	
		USR_GROUP1_AUDIO='<item key="usr.grp.G1.audio"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP1_MICROPHONE='<item key="usr.grp.G1.microphone"		type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_AUDIO='<item key="usr.grp.G2.audio"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_MICROPHONE='<item key="usr.grp.G2.microphone"		type="BOOL" 	min="0" max="1" val="0" />'
	;;
esac

# Individual value(VENDOR) first#
case "$VENDOR" in
	"ITX" )
		USR_INIT_AUTOLOGOUT='<item key="usr.auto_logout"				type="BOOL"		min="0" max="1" val="1" />'
		USR_INIT_AUTOLOGOUT_MIN='<item key="usr.auto_logout_min"			type="UINT"		min="0" max="999" val="10" />'
		
		;;

	"SEQURINET" )
		USR_INIT_AUTOLOGOUT='<item key="usr.auto_logout"				type="BOOL"		min="0" max="1" val="1" />'
		USR_INIT_AUTOLOGOUT_MIN='<item key="usr.auto_logout_min"			type="UINT"		min="0" max="999" val="10" />'
		;;

	"S1" )
		USR_GROUP1_AUDIO='<item key="usr.grp.G1.audio"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_AUDIO='<item key="usr.grp.G2.audio"			type="BOOL" 	min="0" max="1" val="0" />'
		;;

	"S1_JAPAN" )
		USR_GROUP1_AUDIO='<item key="usr.grp.G1.audio"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_AUDIO='<item key="usr.grp.G2.audio"			type="BOOL" 	min="0" max="1" val="0" />'
		;;
		
	"VIDECON" )
		USR_GRP_G3_AUDIO='<item key="usr.grp.G3.audio"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GRP_G3_MICROPHONE='<item key="usr.grp.G3.microphone"		type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP3_SEQURINET='<item key="usr.grp.G3.sequrinet"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_INIT_AUTOLOGOUT='<item key="usr.auto_logout"				type="BOOL"		min="0" max="1" val="1" />'
		USR_INIT_AUTOLOGOUT_MIN='<item key="usr.auto_logout_min"			type="UINT"		min="0" max="999" val="5" />'
		case "$TYPE" in
		"IPXM4B" )
			USR_GRP_G3_AUDIO='<item key="usr.grp.G3.audio"			type="BOOL" 	min="0" max="1" val="0" />'
			USR_GRP_G3_MICROPHONE='<item key="usr.grp.G3.microphone"		type="BOOL" 	min="0" max="1" val="0" />'
			;;
		esac
		;;

	"VIDECON_US" )
		USR_GRP_G3_AUDIO='<item key="usr.grp.G3.audio"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GRP_G3_MICROPHONE='<item key="usr.grp.G3.microphone"		type="BOOL" 	min="0" max="1" val="1" />'
		case "$TYPE" in
		"IPXM4B" )
			USR_GRP_G3_AUDIO='<item key="usr.grp.G3.audio"			type="BOOL" 	min="0" max="1" val="0" />'
			USR_GRP_G3_MICROPHONE='<item key="usr.grp.G3.microphone"		type="BOOL" 	min="0" max="1" val="0" />'
			;;
		esac
		;;
	
	"G4S" )
		USR_GROUP0_SEQURINET='<item key="usr.grp.G0.sequrinet"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP1_SEQURINET='<item key="usr.grp.G1.sequrinet"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP2_SEQURINET='<item key="usr.grp.G2.sequrinet"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP3_SEQURINET='<item key="usr.grp.G3.sequrinet"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP4_SEQURINET='<item key="usr.grp.G4.sequrinet"			type="BOOL" 	min="0" max="1" val="0" />'
		;;
	
	"ORION" )
		USR_GROUP0_SEQURINET='<item key="usr.grp.G0.sequrinet"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP1_SEQURINET='<item key="usr.grp.G1.sequrinet"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP2_SEQURINET='<item key="usr.grp.G2.sequrinet"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP3_SEQURINET='<item key="usr.grp.G3.sequrinet"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP4_SEQURINET='<item key="usr.grp.G4.sequrinet"			type="BOOL" 	min="0" max="1" val="0" />'
		;;
	
	"TAKENAKA" )
		USR_GROUP1_ARCHIVE='<item key="usr.grp.G1.archive"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP1_SYS_SETUP='<item key="usr.grp.G1.sys_setup"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP1_REC_SETUP='<item key="usr.grp.G1.rec_setup"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP1_REMOTE='<item key="usr.grp.G1.remote"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP1_SHUTDOWN='<item key="usr.grp.G1.shutdown"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_EVENT='<item key="usr.grp.G2.event"				type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_AUDIO='<item key="usr.grp.G2.audio"				type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_MICROPHONE='<item key="usr.grp.G2.microphone"		type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_REMOTE='<item key="usr.grp.G2.remote"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP2_SHUTDOWN='<item key="usr.grp.G2.shutdown"			type="BOOL" 	min="0" max="1" val="0" />'
		;;

	"ZICOM" )
		USR_INIT_AUTOLOGOUT='<item key="usr.auto_logout"				type="BOOL"		min="0" max="1" val="1" />'
		USR_INIT_AUTOLOGOUT_MIN='<item key="usr.auto_logout_min"			type="UINT"		min="0" max="999" val="10" />'
		USR_GROUP1_ARCHIVE='<item key="usr.grp.G1.archive"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP1_SYS_SETUP='<item key="usr.grp.G1.sys_setup"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP1_REC_SETUP='<item key="usr.grp.G1.rec_setup"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP1_REMOTE='<item key="usr.grp.G1.remote"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP1_SHUTDOWN='<item key="usr.grp.G1.shutdown"			type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_EVENT='<item key="usr.grp.G2.event"				type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_AUDIO='<item key="usr.grp.G2.audio"				type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_MICROPHONE='<item key="usr.grp.G2.microphone"		type="BOOL" 	min="0" max="1" val="0" />'
		USR_GROUP2_REMOTE='<item key="usr.grp.G2.remote"			type="BOOL" 	min="0" max="1" val="1" />'
		USR_GROUP2_SHUTDOWN='<item key="usr.grp.G2.shutdown"			type="BOOL" 	min="0" max="1" val="0" />'
		;;
esac

#####  SYSDB USER START  ##########################################################################################
NF_SYSDB_USER=(
'<usr>'
'<item key="usr.grp.GCNT"				type="UINT" 	min="0" max="5" val="5" />'
'<item key="usr.grp.G0.name"				type="STRING" 	min="0" max="128" val="ADMIN" />'
'<item key="usr.grp.G0.permission"					type="UINT" 	min="0" max="2147483647" val="2147483647" />'
'<item key="usr.grp.G0.desc"				type="STRING" 	min="0" max="64" val="" />'
'<item key="usr.grp.G0.sys_setup"		type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.search"			type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.archive"			type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.rec_setup"		type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.event"			type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.audio"			type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.microphone"		type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.remote"			type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.ptz"				type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.shutdown"			type="BOOL" 	min="0" max="1" val="1" />'
'<item key="usr.grp.G0.cvt_disp"			type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GROUP0_SEQURINET"

'<item key="usr.grp.G1.name"				type="STRING" 	min="0" max="128" val="MANAGER" />'
'<item key="usr.grp.G1.permission"					type="UINT" 	min="0" max="2147483647" val="4863" />'
'<item key="usr.grp.G1.desc"				type="STRING" 	min="0" max="64" val="" />'
"$USR_GROUP1_SYS_SETUP"
'<item key="usr.grp.G1.search"			type="BOOL" 	min="0" max="1" val="1" />'
"$USR_GROUP1_ARCHIVE"
"$USR_GROUP1_REC_SETUP"
'<item key="usr.grp.G1.event"			type="BOOL" 	min="0" max="1" val="1" />'
"$USR_GROUP1_AUDIO"
"$USR_GROUP1_MICROPHONE"
"$USR_GROUP1_REMOTE"
'<item key="usr.grp.G1.ptz"		type="BOOL" 	min="0" max="1" val="1" />'
"$USR_GROUP1_SHUTDOWN"
'<item key="usr.grp.G1.cvt_disp"			type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GROUP1_SEQURINET"

'<item key="usr.grp.G2.name"				type="STRING" 	min="0" max="128" val="USER" />'
'<item key="usr.grp.G2.permission"					type="UINT" 	min="0" max="2147483647" val="737" />'
'<item key="usr.grp.G2.desc"				type="STRING" 	min="0" max="64" val="" />'
'<item key="usr.grp.G2.sys_setup"		type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GROUP2_SEARCH"
'<item key="usr.grp.G2.archive"			type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G2.rec_setup"		type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GROUP2_EVENT"
"$USR_GROUP2_AUDIO"
"$USR_GROUP2_MICROPHONE"
"$USR_GROUP2_REMOTE"
'<item key="usr.grp.G2.ptz"		type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GROUP2_SHUTDOWN"
'<item key="usr.grp.G2.cvt_disp"			type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GROUP2_SEQURINET"

'<item key="usr.grp.G3.name"				type="STRING" 	min="0" max="128" val="VIEWER" />'
'<item key="usr.grp.G3.permission"					type="UINT" 	min="0" max="2147483647" val="225" />'
'<item key="usr.grp.G3.desc"				type="STRING" 	min="0" max="64" val="" />'
'<item key="usr.grp.G3.sys_setup"					type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G3.search"						type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G3.archive"						type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G3.rec_setup"					type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G3.event"						type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GRP_G3_AUDIO"
"$USR_GRP_G3_MICROPHONE"
'<item key="usr.grp.G3.remote"						type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G3.ptz"							type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G3.shutdown"					type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G3.cvt_disp"					type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GROUP3_SEQURINET"

'<item key="usr.grp.G4.name"				type="STRING" 	min="0" max="128" val="LOGOFF" />'
'<item key="usr.grp.G4.permission"					type="UINT" 	min="0" max="2147483647" val="0" />'
'<item key="usr.grp.G4.desc"				type="STRING" 	min="0" max="64" val="" />'
'<item key="usr.grp.G4.sys_setup"		type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.search"			type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.archive"			type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.rec_setup"		type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.event"			type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.audio"			type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.microphone"		type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.remote"			type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.ptz"							type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.shutdown"			type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.G4.cvt_disp"			type="BOOL" 	min="0" max="1" val="0" />'
'<item key="usr.grp.anonymous"			type="BOOL" 	min="0" max="1" val="0" />'
"$USR_GROUP4_SEQURINET"

'<item key="usr.UCNT"								type="UINT" 	min="0" max="120" val="1" />'

"$USR_INIT_AUTOLOGOUT"
"$USR_INIT_AUTOLOGOUT_MIN"
)

### USER CNT -> 120 #####################################
#
# Common Value #
for((i=0; i<120; i++))
do
	if [ $i -eq 0 ];
	then
		USR_NAME=$(printf '<item key="usr.U%d.name"								type="STRING" 	min="0" max="32" val="ADMIN" />' "$i")
 		USR_PASS=$(printf '<item key="usr.U%d.pass"								type="STRING" 	min="0" max="32" val="1234" />' "$i")
 		USR_GRPNAME=$(printf '<item key="usr.U%d.grpname"						type="STRING" 	min="0" max="32" val="ADMIN" />' "$i")
		
	else
		USR_NAME=$(printf '<item key="usr.U%d.name"								type="STRING" 	min="0" max="32" val="" />' "$i")
 		USR_PASS=$(printf '<item key="usr.U%d.pass"								type="STRING" 	min="0" max="32" val="" />' "$i")
 		USR_GRPNAME=$(printf '<item key="usr.U%d.grpname"						type="STRING" 	min="0" max="32" val="" />' "$i")
	fi
	
	USR_USE_PINCODE=$(printf '<item key="usr.U%d.use_pin"					type="BOOL" 	min="0" max="1" val="1" />' "$i")
	USR_PINCODE_NUMBER=$(printf '<item key="usr.U%d.pin_number"				type="STRING" 	min="0" max="32" val="" />' "$i")
	USR_QUESTION0=$(printf '<item key="usr.U%d.question0"				type="UINT" 	min="" max="" val="" />' "$i")
	USR_QUESTION1=$(printf '<item key="usr.U%d.question1"				type="UINT" 	min="" max="" val="" />' "$i")
	USR_QUESTION2=$(printf '<item key="usr.U%d.question2"				type="UINT" 	min="" max="" val="" />' "$i")
	USR_ANSWER0=$(printf '<item key="usr.U%d.answer0"				type="STRING" 	min="0" max="64" val="" />' "$i")
	USR_ANSWER1=$(printf '<item key="usr.U%d.answer1"				type="STRING" 	min="0" max="64" val="" />' "$i")
	USR_ANSWER2=$(printf '<item key="usr.U%d.answer2"				type="STRING" 	min="0" max="64" val="" />' "$i")
 	USR_EMAIL=$(printf '<item key="usr.U%d.email"								type="STRING" 	min="0" max="64" val="" />' "$i")
 	USR_EMAIL_NOTIFY=$(printf '<item key="usr.U%d.email_notify"					type="BOOL" 	min="0" max="1" val="0" />' "$i")
 	USR_EMAIL_CERTIFICATION=$(printf '<item key="usr.U%d.email_certification"	type="BOOL" 	min="0" max="1" val="0" />' "$i")
 	USR_DESC=$(printf '<item key="usr.U%d.desc"									type="STRING" 	min="0" max="64" val="" />' "$i")
	USR_GROUP=$(printf '<item key="usr.U%d.group"								type="UINT" 	min="0" max="4" val="0" />' "$i")
 	USR_COVERT=$(printf '<item key="usr.U%d.covert"								type="STRING" 	min="32" max="32" val="00000000000000000000000000000000" />' "$i")
 	USR_PW_LAST_CHANGED=$(printf '<item key="usr.U%d.pw_last_changed"			type="UINT" 	min="0" max="" val="0" />' "$i")
 	USR_EXPIRED_CHECK=$(printf '<item key="usr.U%d.expired_check"				type="UINT" 	min="0" max="" val="0" />' "$i")
 	USR_PHONE=$(printf '<item key="usr.U%d.phone"								type="STRING" 	min="0" max="32" val="" />' "$i")
	USR_PHONE_NOTIFY=$(printf '<item key="usr.U%d.phone_notify"					type="BOOL" 	min="0" max="1" val="0" />' "$i")
  	USR_PHONE_CERTIFICATION=$(printf '<item key="usr.U%d.phone_certification"	type="BOOL" 	min="0" max="1" val="0" />' "$i")
  	USR_EMAIL_SERV=$(printf '<item key="usr.U%d.email_serv"						type="UINT"		min="0" max="" val="0" />' "$i")
  	USR_PASS_ENCRYPTION=$(printf '<item key="usr.U%d.pass_encryption"			type="STRING" 	min="0" max="128" val="" />' "$i")
    USR_CERTI_POPUP_HIDE=$(printf '<item key="usr.U%d.certi_popup_hide"         type="BOOL"     min="0" max="1" val="0" />' "$i")
    USR_PERMISSION=$(printf '<item key="usr.U%d.permission"					    type="UINT" 	min="0" max="2147483647" val="2147483647" />' "$i")
    USR_INIT_PW_CHANGED=$(printf '<item key="usr.U%d.init_pw_changed"			type="UINT" 	min="0" max="" val="0" />' "$i")

# Individual value (VENDOR) second#
	case "$VENDOR" in
		"DAYOU" )
    		USR_CERTI_POPUP_HIDE=$(printf '<item key="usr.U%d.certi_popup_hide"         type="BOOL"     min="0" max="1" val="1" />' "$i")
		;;
	esac


	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_QUESTION0}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_QUESTION1}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_QUESTION2}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_ANSWER0}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_ANSWER1}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_ANSWER2}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_NAME}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_PASS}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_USE_PINCODE}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_PINCODE_NUMBER}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_EMAIL}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_EMAIL_NOTIFY}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_EMAIL_CERTIFICATION}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_DESC}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_GROUP}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_GRPNAME}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_COVERT}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_PW_LAST_CHANGED}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_EXPIRED_CHECK}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_PHONE}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_PHONE_NOTIFY}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_PHONE_CERTIFICATION}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_EMAIL_SERV}" )
	NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_PASS_ENCRYPTION}" )
    NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_CERTI_POPUP_HIDE}" )
    NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_PERMISSION}" )
    NF_SYSDB_USER=( "${NF_SYSDB_USER[@]}" "${USR_INIT_PW_CHANGED}" )
done

case "$VENDOR" in
	"eneo" )
		USR_U0_NAME='<item key="usr.U0.name"					type="STRING" 	min="0" max="32" val="admin" />'
		USR_U0_PASSWORD='<item key="usr.U0.pass"                            type="STRING"   min="0" max="32" val="admin" />'
	;;
esac

####   GENERATOR START  ##########################################################################################
for((i=0;i<${#NF_SYSDB_USER[@]};i++))
do
	if [ "${NF_SYSDB_USER[i]}" = "" ]; then
		echo ${NF_SYSDB_USER[i-1]} ".OK"
		echo "### ERROR : The next item is not exist."
		exit 1
	fi

	echo ${NF_SYSDB_USER[i]} >> nf_sysdb.conf
done

	echo '</usr>' >> nf_sysdb.conf                          
####   GENERATOR END    ##########################################################################################