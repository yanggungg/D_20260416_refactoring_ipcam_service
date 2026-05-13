#!/bin/sh

############################################################################
# WEBRA HOST 소스를 제품 별로 동기화 시키기 위한 TOOL 입니다.
# 관련자만 사용하기 바랍니다.
#
# 기본적으로, 각 제품의 대해서
# sysman
# include
# 위 디렉토리를 제품 별로 아래의 이름으로 링크로 갖고 있어야 합니다.
# ./sysman_ipx
# ./sysman_hdy
# ./sysman_hdi
# ./sysman_dvr
# ./include_ipx
# ./include_hdy
# ./include_hdi
# ./include_dvr
# 
# 실행하면 stdout 으로 필요한 명령이 출력되므로, sh로 pipe 시켜주면 실행됩니다.
############################################################################

# colordiff.pl 이 존재하면 지원합니다.
if test -x `which colordiff.pl` ; then
  DIFF=colordiff.pl
else
  DIFF=diff
fi

if [ -e make.env ] 
then
  . ./make.env
fi

echo "echo TARGET_MODEL=$TARGET_MODEL"

###############################################
all_target=(hdy hdi dvr)

if [[ $TARGET_MODEL =~ .*SNF.* ]] ; then
  echo "echo IPX"
  all_target=(hdy hdi dvr)
elif [[ $TARGET_MODEL =~ IPX ]] ; then
  echo "echo IPX"
  all_target=(hdy hdi dvr)
elif [[ $TARGET_MODEL =~ HDI ]] ; then
  echo "echo HDI"
  all_target=(ipx hdy dvr)
elif [[ $TARGET_MODEL =~ HDY ]] ; then
  echo "echo HDY"
  all_target=(ipx hdi dvr)
elif [[ $TARGET_MODEL =~ ANFD ]] ; then
  echo "echo DVR"
  all_target=(ipx hdi hdy)
elif [[ $TARGET_MODEL =~ ATMD ]] ; then
  echo "echo DVR"
  all_target=(ipx hdi hdy)
fi
###############################################

function find_diff {
  if [ "$2" == "nocolor" ] ; then
    DIFF=diff
  fi

  if [ "$1" == "all" ]
  then
    for t in ${all_target[@]}; do
      find -name 'nf_webra*.[ch]' -printf "$DIFF -ur %h/%f %h_$t/%f\n"
    done
  else
    find -name 'nf_webra*.[ch]' -printf "$DIFF -ur %h/%f %h_$1/%f\n"
  fi

}

function find_cp {
  if [ "$1" == "all" ]
  then
    for t in ${all_target[@]}; do
      find -name 'nf_webra*.[ch]' -printf "cp %p %h_$t/%f\n"
    done
  else
    find -name 'nf_webra*.[ch]' -printf "cp %p %h_$1/%f\n"
  fi
}

if [ "$2" == "" ] 
then
  TARGET=${all_target[0]}
else
  TARGET=$2
fi

case "$1" in
  "diff")
    find_diff $TARGET
  ;;
  "diff2")
    find_diff $TARGET nocolor
  ;;
  "cp")
    find_cp $TARGET
  ;;
  *)
   echo "ipx webra host sync tool"
   echo "  Usage: $0 op[diff|diff2|cp] {target[ipx|hdy|hdi|dvr|all]} - default for ipx"
  ;;
esac

