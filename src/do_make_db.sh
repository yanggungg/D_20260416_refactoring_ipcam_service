#!/bin/sh

source ./make_db.env

function error_msg_if_fail
{
	if [ $1 != 0 ];	then
		echo "### ERROR : $2"
		exit 1
	fi
}

rm -rf ../data/nf_sysdb_*.conf
rm -rf nf_sysdb.conf
sh make_db.sh $TYPE $MODEL $VENDOR $SIGNAL_TYPE
xmllint nf_sysdb.conf > tmp
error_msg_if_fail $? "Make DB fail."
mv nf_sysdb.conf ../data/nf_sysdb.conf
  