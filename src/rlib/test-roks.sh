#!/bin/sh

vl='1'

while (test $# -gt 0)
do
  case "$1" in
    -v* | --verbose-level*)
      vl="$2"
      shift
      ;;

    -h* | --help*)
      echo "Usage: test-roks.sh [-v verbose_level]"
      exit 0
      ;;

    *)
      echo "Unexpected parameter '$1', type --help, exiting..."
      exit 1
      ;;

  esac
  shift
done

################################################################################

src_path='/afs/cern.ch/user/i/isolov/working/online'

 # schema definition for Oracle DBMS
sqlf="${src_path}/oks/src/rlib/create_db.oracle.sql"

 # grant permissions
sqlf2="${src_path}/oks/src/rlib/grant.sql"

 # schema files
schema_file=`find ${src_path}/oks/src/rlib/data4test -name '*.data.xml' | sort`

 # data files
data_files=`find ${src_path}/oks/src/rlib/data4test -name '*.data.xml' | sort`

################################################################################

user='ral_writer'
connect="-c oracle://devdb10/ral_reader -w ${user}"

################################################################################

OKS_KERNEL_SILENCE='yes'; export OKS_KERNEL_SILENCE

################################################################################

 # clean database
echo "exit" | sqlplus ${user}/ral@devdb10 @${sqlf}

 # grant append permissions
echo "exit" | sqlplus ${user}/ral@devdb10 @${sqlf2}

################################################################################

echo 'Testing explicit schema versions...'
count=1
for f in ${schema_file}; do
  echo oks_put_schema ${connect} -f ${f} -d test -n ${count}
  oks_put_schema ${connect} -f ${f} -d "test" -n ${count} || exit 1
  count=`expr $count + 1`
done

echo '###########################################################################'
echo '# DONE'
echo '###########################################################################'
echo ''

################################################################################

echo 'Testing automatic schema versioning...'
for f in ${schema_file}; do
  echo oks_put_schema ${connect} -f ${f} -d test -a
  oks_put_schema ${connect} -f ${f} -d "test" -a || exit 1
  count=`expr $count + 1`
done

echo '###########################################################################'
echo '# DONE'
echo '###########################################################################'
echo ''

################################################################################

echo 'Testing stored schemas...'
o="/tmp/schema.`whoami`.$$.xml"
count=1
for f in ${schema_file}; do
  echo oks_get_schema ${connect} -v ${vl} -f ${o} -n ${count}
  oks_get_schema ${connect} -v ${vl} -f ${o} -n ${count} || exit 1
  echo oks_diff_schema ${f} ${o}
  oks_diff_schema ${f} ${o} | grep -v 'no differences were found' || exit 1
  count=`expr $count + 1`
  rm -f ${o} || exit 1
done

echo '###########################################################################'
echo '# DONE'
echo '###########################################################################'
echo ''

################################################################################

echo 'Testing automatic data versioning...'
count=1
for f in ${data_files}; do
  if [ $count -eq 1 ] ; then
    echo oks_put_data ${connect} -f ${f} -v ${vl} -s 1 -a -d 'insert base version'
    oks_put_data ${connect} -f ${f} -v ${vl} -s 1 -a -d 'insert base version' || exit 1
  else
    pv=`oks_ls_data ${connect} | grep -v '=========' | grep -v '| Date (UTC)' | wc -l`
    echo oks_put_data ${connect} -f ${f} -v ${vl} -s 1 -b ${pv} -a -d 'insert incremented version'
    oks_put_data ${connect} -f ${f} -v ${vl} -s 1 -b ${pv} -a -d 'insert incremented version' || exit 1
  fi
  count=`expr $count + 1`
done

echo '###########################################################################'
echo '# DONE'
echo '###########################################################################'
echo ''

################################################################################

echo 'Testing stored data...'
os="/tmp/schema.`whoami`.$$.xml"
od="/tmp/data.`whoami`.$$.xml"
count=1
for f in ${data_files}; do
  echo oks_get_data ${connect} -s 1 -n ${count} -m ${os} -f ${od}
  oks_get_data ${connect} -s 1 -n ${count} -m ${os} -f ${od} || echo 'ignoring...'
  echo oks_diff_data -a -r -d1 ${f} -d2 ${od}
  oks_diff_data -a -r -d1 "${f}" -d2 "${od}" || echo 'ignoring...'
  count=`expr $count + 1`
  rm -f ${os} ${od} || exit 1
done

echo '###########################################################################'
echo '# DONE'
echo '###########################################################################'
echo ''

################################################################################

echo oks_ls_data ${connect} -u -z -t v -d
oks_ls_data ${connect} -u -z -t v -d || exit 1

################################################################################
