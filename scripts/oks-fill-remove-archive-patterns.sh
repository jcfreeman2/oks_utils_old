#!/bin/sh

########################################################################################################################

verbose=1
out_file=''
pattern_file_name='remove-from-oks-archive.txt'

########################################################################################################################

# parse commnad line

while (test $# -gt 0)
do
  case "$1" in
    -v | --verb*)
     verbose="$2" ; shift ;
     ;;

    -o | --out-file*)
     out_file="$2" ; shift ;
     ;; 

    -h* | --he*)
     echo 'Usage: oks-create-new-base-version.sh -o out_file [-h] [-v]'
     echo ''
     echo 'Arguments/Options:'
     echo '   -o | --out-file file    name of file where search parrents to be stored'
     echo '   -v | --verbose level    switch on verbose output'
     echo '   -h | --help             print this message'
     echo ''
     echo 'Description:'
     echo "   The utility is looking for the ${pattern_file_name} files under TDAQ_INST_PATH and"
     echo '   and merges their content into out file.'
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

if [ -z "${out_file}" ]; then echo "ERROR: out_file is required"; exit 1 ; fi
if [ -z "${TDAQ_INST_PATH}" ]; then echo "ERROR: TDAQ_INST_PATH is not set"; exit 1 ; fi

########################################################################################################################

for f in `find ${TDAQ_INST_PATH} -name ${pattern_file_name} -print`; do
  if [ $verbose -gt 1 ]; then echo "- add file ${f}"; fi
  cat ${f} >> ${out_file}
done

########################################################################################################################

exit 0

########################################################################################################################
