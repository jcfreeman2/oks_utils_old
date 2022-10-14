#!/bin/sh

release=`echo ${QUERY_STRING} | awk -F '\&' '{print $1}'`
connect=`echo ${QUERY_STRING} | awk -F '\&' '{print $2}'`
owner=`echo ${QUERY_STRING} | awk -F '\&' '{print $3}'`
schema=`echo ${QUERY_STRING} | awk -F '\&' '{print $4}'`
data=`echo ${QUERY_STRING} | awk -F '\&' '{print $5}'`
partition=`echo ${QUERY_STRING} | awk -F '\&' '{print $6}'`

###############################################################################################################################

dir="/tmp/oks-archive.$$"

if [ -z "$partition" ]
then
  data_file="$schema.$data.data.xml"
else
  data_file="$partition.$schema.$data.data.xml"
fi
schema_file="$schema.schema.xml"
archive_file="$schema.$data.tar.gz"
out_file="$dir/out.log"

rm -rf ${dir} > /dev/null
mkdir -p ${dir} > /dev/null
cd $dir >> ${out_file} 2>&1 || (echo "cd $dir failed" >> ${out_file}; exit 1)

###############################################################################################################################

TNS_ADMIN=/afs/cern.ch/project/oracle/admin; export TNS_ADMIN

# setup tdaq release

echo "Setup $release release ..." >> ${out_file}
. /afs/cern.ch/atlas/project/tdaq/cmt/$release/installed/setup.sh >> ${out_file} 2>&1 || (echo "Failed to setup $release DAQ/HLT-I release" >> ${out_file}; exit 1)

CORAL_AUTH_PATH='/afs/cern.ch/user/i/isolov/www/cgi-bin'
export CORAL_AUTH_PATH;

# get data

query=''

if [ ! -z "$partition" ]
then
  query=`echo '(all (object-id "XXX" =))' | sed "s/XXX/$partition/"`
  echo Execute oks_get_data -c $connect -w $owner -s $schema -n $data -q Partition "$query" -r 1000 -f ${data_file} -m ${schema_file} >> ${out_file}
  oks_get_data -c $connect -w $owner -s $schema -n $data -q Partition "$query" -r 1000 -f ${data_file} -m ${schema_file} >> ${out_file} 2>&1 || (echo "<b>oks_get_data</b> failed: $?" >> ${out_file}; rm -f ${data_file} ${schema_file}; exit 2)
else
  echo "Execute oks_get_data -c $connect -w $owner -s $schema -n $data -f ${data_file} -m ${schema_file}" >> ${out_file}
  oks_get_data -c $connect -w $owner -s $schema -n $data -f ${data_file} -m ${schema_file} >> ${out_file} 2>&1 || (echo "<b>oks_get_data</b> failed: $?" >> ${out_file}; rm -f ${data_file} ${schema_file}; exit 2)
fi

if [ -f ${data_file} ]
then
  cd $dir >> ${out_file} 2>&1 || (echo "cd $dir failed" >> ${out_file}; exit 1)
  echo 'Compress files' >> ${out_file}
  echo "tar cvf - ${schema_file} ${data_file} *.log | gzip -c " >> ${out_file}
  tar cvf - ${schema_file} ${data_file} *.log 2>> ${out_file} | gzip -c > "$archive_file" 2>> ${out_file} 
  if [ -f "$archive_file" ]
  then
    echo 'Content-Type: application/x-download'
    echo "Content-Disposition: attachment; filename=$archive_file"
    echo ''
    cat "/$dir/$archive_file"
    rm -rf $dir
    exit 0
  fi
fi

echo 'Content-type: text/html'
echo ''
echo '<html> <head> <title>Download OKS file failed</title> </head>'
echo '<body><pre>'
cat ${out_file}
echo '</pre></body></html>'

rm -rf ${dir}
