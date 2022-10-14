#!/bin/sh

TNS_ADMIN=/afs/cern.ch/project/oracle/admin; export TNS_ADMIN

# setup tdaq release

. /afs/cern.ch/atlas/project/tdaq/cmt/$1/installed/setup.sh > /dev/null || (echo "Failed to setup $1 DAQ/HLT-I release"; exit 1)

#PATH=/afs/cern.ch/user/i/isolov/working/online/installed/i686-slc3-gcc323-opt/bin:$PATH
#export PATH

#LD_LIBRARY_PATH=/afs/cern.ch/user/i/isolov/working/online/installed/i686-slc3-gcc323-opt/lib:$LD_LIBRARY_PATH
#export LD_LIBRARY_PATH

CORAL_AUTH_PATH='/afs/cern.ch/user/i/isolov/www/cgi-bin'
export CORAL_AUTH_PATH;

# get table

cmd_args="-c $2 -w $3"

shift
shift
shift

oks_ls_data ${cmd_args} $* 2>&1 || (echo "<b>oks_ls_data</b> failed: $?"; exit 2) #| grep -v '====================' | grep -v '| Description'

exit $?
