#!/bin/sh

########################################################################################################################

verbose=1
connect_str=''
schema_name=''
oks_file='daq/sw/tags.data.xml'

########################################################################################################################

# parse commnad line

while (test $# -gt 0)
do
  case "$1" in

    -c | --connect-string*)
      connect_str="$2" ; shift ;
      ;;

    -w | --working-schema*)
      schema_name="$2" ; shift ;
      ;;

    -v | --verb*)
      verbose="$2" ; shift ;
      ;;

    -f | --file*)
      oks_file="$2" ; shift ;
      ;;

    -h* | --he*)
      echo 'Usage: oks-create-new-base-version.sh -c connect_string -w schema_name [-f oks_file] [-h] [-v level]'
      echo ''
      echo 'Arguments/Options:'
      echo '   -c | --connect-string connect_str    database connection string'
      echo '   -w | --working-schema schema_name    name of relational database working schema'
      echo "   -v | --verbose level                 set level for verbose output [default=${verbose}]"
      echo "   -f | --file oks_file                 name of oks file to be used for base version [default=${oks_file}]"
      echo '   -h | --help                          print this message'
      echo ''
      echo 'Description:'
      echo '   Creates new base version in the relational database from schema and objects of user-defined file.'
      echo "   New objects and classes will be added to the base version during it's usage for incremental archiving."
      echo ''
      exit 0
      ;;

    *)
      echo "Unexpected parameter '$1', type --help, exiting..."
      exit 1
      ;;

  esac
  shift
done

if [ -z "${connect_str}" ]; then echo "ERROR: connect string is required"; exit 1 ;fi
if [ -z "${schema_name}" ]; then echo "ERROR: working schema is required"; exit 1 ;fi

########################################################################################################################

common_params="-c ${connect_str} -w ${schema_name} -v ${verbose} -f ${oks_file}"

echo oks_put_schema ${common_params} -a
oks_put_schema ${common_params} -a || ( echo 'ERROR: oks_put_schema failed, exiting...' ; exit 1 ; )

echo oks_put_data ${common_params} -l -a
oks_put_data ${common_params} -l -a || ( echo 'ERROR: oks_put_data failed, exiting...' ; exit 1 ; )

########################################################################################################################
