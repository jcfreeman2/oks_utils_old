#!/bin/sh

###############################################################################################################################

dir="/tmp/oks-archivei-cmp.$$"

df1="$4.data.xml"
sf1="$4.schema.xml"
df2="$5.data.xml"
sf2="$5.schema.xml"
out_file="$dir/out.log"

rm -rf ${dir} > /dev/null
mkdir -p ${dir} > /dev/null
cd $dir >> ${out_file} 2>&1 || (echo "cd $dir failed" >> ${out_file}; exit 1)

###############################################################################################################################

TNS_ADMIN=/afs/cern.ch/project/oracle/admin; export TNS_ADMIN

# setup tdaq release

echo "Setup $release release ..." >> ${out_file}
. /afs/cern.ch/atlas/project/tdaq/cmt/$1/installed/setup.sh >> ${out_file} 2>&1 || (echo "Failed to setup $1 DAQ/HLT-I release" >> ${out_file}; exit 1)

CORAL_AUTH_PATH='/afs/cern.ch/user/i/isolov/www/cgi-bin'
export CORAL_AUTH_PATH;

###############################################################################################################################

# get data

sv1=`echo $4 | sed 's/\([0-9]*\).*/\1/'`
dv1=`echo $4 | sed 's/\([0-9]*\)\.\([0-9]*\)*.*/\2/'`

sv2=`echo $5 | sed 's/\([0-9]*\).*/\1/'`
dv2=`echo $5 | sed 's/\([0-9]*\)\.\([0-9]*\)*.*/\2/'`

query=``
if [ ! -z "$6" ]
then
  query=`echo '(all (object-id "XXX" =))' | sed "s/XXX/$6/"`
fi

if [ ! -z "$query" ]
then
  echo "Execute oks_get_data -c $2 -w $3 -s $sv1 -n $dv1 -q Partition "$query" -r 1000 -f ${df1} -m ${sf1}" >> ${out_file}
  oks_get_data -c $2 -w $3 -s $sv1 -n $dv1 -q Partition "$query" -r 1000 -f ${df1} -m ${sf1} >> ${out_file} 2>&1 || (echo "<b>oks_get_data</b> failed: $?" >> ${out_file}; echo "Check partition <b>$6</b> was stored in this version!" >> ${out_file}; rm -f ${df1} ${sf1})
else
  echo "Execute oks_get_data -c $2 -w $3 -s $sv1 -n $dv1 -f ${df1} -m ${sf1}" >> ${out_file}
  oks_get_data -c $2 -w $3 -s $sv1 -n $dv1 -f ${df1} -m ${sf1} >> ${out_file} 2>&1 || (echo "<b>oks_get_data</b> failed: $?" >> ${out_file}; rm -f ${df1} ${sf1})
fi

if [ ! -f ${df1} ]
then
  cat ${out_file}
  rm -rf $dir
  exit 1
fi

if [ ! -z "$query" ]
then
  echo "Execute oks_get_data -c $2 -w $3 -s $sv2 -n $dv2 -q Partition "$query" -r 1000 -f ${df2} -m ${sf2}" >> ${out_file}
  oks_get_data -c $2 -w $3 -s $sv2 -n $dv2 -q Partition "$query" -r 1000 -f ${df2} -m ${sf2} >> ${out_file} 2>&1 || (echo "<b>oks_get_data</b> failed: $?" >> ${out_file}; echo "Check partition <b>$6</b> was stored in this version!" >> ${out_file}; rm -f ${df2} ${sf2})
else
  echo "Execute oks_get_data -c $2 -w $3 -s $sv2 -n $dv2 -f ${df2} -m ${sf2}" >> ${out_file}
  oks_get_data -c $2 -w $3 -s $sv2 -n $dv2 -f ${df2} -m ${sf2} >> ${out_file} 2>&1 || (echo "<b>oks_get_data</b> failed: $?" >> ${out_file}; rm -f ${df2} ${sf2})
fi

if [ ! -f ${df2} ]
then
  cat ${out_file}
  rm -rf $dir
  exit 2
fi

oks_diff_data -a -r -d1 ${df1} -d2 ${df2}

rm -rf $dir

exit 0
