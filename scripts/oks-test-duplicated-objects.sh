#!/bin/sh

verbose='no'
file="/tmp/oks-test-duplicated-objects.$$.unsorted"
file2="/tmp/oks-test-duplicated-objects.$$.num-of-errors"
file3="/tmp/oks-test-duplicated-objects.$$.ignore"
file4="/tmp/oks-test-duplicated-objects.$$.paths"

# parse command line

is_reg_exp='no'

while (test $# -gt 0)
do
  if [ "${is_reg_exp}" == 'yes' ]; then

    if [ $verbose == 'yes' ]; then echo " add regular expression \'$1\' to list of ignored files "; fi
    echo "$1" >> ${file3}

  else 

    case "$1" in

      -v | --verb*)
        verbose='yes'
       ;;

      -s | --skip*)
        is_reg_exp='yes'
       ;;

      -h* | --he*)
        echo 'Usage: oks-test-duplicated-objects.sh [--help] [--verbose] [--skip-files reg-exp*]'
        echo ''
        echo 'Arguments/Options:'
        echo '   -v | --verbose             verbose output'
        echo '   -h | --help                print this message'
        echo '   -s | --skip-files r1 ...   list of regular expressions to ignore files'
        echo ''
        echo 'Description:'
        echo '   Reports duplicated oks objects looking to all *.data.xml files'
        echo '   in repositories defined by the TDAQ_DB_PATH environment variable.'
        echo ''
        exit 0
        ;;

      *)
        echo "Unexpected parameter '$1', type --help, exiting..."
        exit 1
        ;;

    esac

  fi
  shift
done

if [ $verbose == 'yes' ]; then echo "looking for oks data files in TDAQ_DB_PATH=${TDAQ_DB_PATH}"; fi

oks-fill-remove-archive-patterns.sh -o ${file3}

touch "${file4}"

test=`echo ${TDAQ_DB_PATH} | sed 's/:/ /g'`
for d in ${test}; do

    # check if this path was processed already
  test_token=`grep "$d" ${file4}`
  if [ "${test_token}" == "$d" ]
  then
    if [ $verbose == 'yes' ]; then echo "Warning: directory $d was already processed, check TDAQ_DB_PATH value"; fi
    continue;
  fi
  echo "$d" >> "${file4}"

    # process files from path
  if [ -d ${d} ]
  then
    if [ $verbose == 'yes' ]; then echo "process files from directory '$d' ..."; fi
    for f in `find $d -follow -type f -name '*.data.xml' -print`; do
      if [ "${is_reg_exp}" == 'yes' ] ; then
        f1=`echo $f | grep -v -f ${file3}`
        if [ -z "${f1}" ] ; then
          if [ $verbose == 'yes' ] ; then echo "- skip file $f"; fi
          continue
        fi
      fi
      if [ $verbose == 'yes' ]; then echo "- process file $f"; fi
      grep '<obj class=' $f | sed 's/<obj class=//;s/id=//;s/>//' | awk --assign FFF="$f" '{ printf "%s \"%s\"\n", $0, FFF }' >> ${file}
    done
  else
    if [ $verbose == 'yes' ]; then echo "Warning: directory $d does not exist"; fi
  fi
done

f=''
c=''
o=''
p=''
err_count='0'

cat ${file} | sort | while read line
do
  c1=`echo $line | awk -F '\"' '{ print $2 }'`
  o1=`echo $line | awk -F '\"' '{ print $4 }'`
  f1=`echo $line | awk -F '\"' '{ print $6 }'`

  if [ "${c}" == "${c1}" ] 
  then
    if [ "${o}" == "${o1}" ]
    then
      if [ -z "${p}" ]
      then
        err_count=`expr $err_count + 1`
        echo "$err_count" > "${file2}"
        echo "Error [${err_count}]: object \"${o}@${c}\" is defined in several files:"
        echo " - ${f}"
        p='1'
      fi
      echo " - ${f1}"
    else
      o="${o1}"
      f="${f1}"
      p=''
    fi
  else
    c="${c1}"
    o="${o1}"
    f="${f1}"
    p=''
  fi
done

err_count=`cat ${file2}`

rm -f "${file} ${file2} ${file3} ${file4}"

if [ $err_count -eq 0 ]
then
  if [ $verbose == 'yes' ]; then echo "Test is OK"; fi
else
  if [ $verbose == 'yes' ]; then echo "Test failed ($err_count errors were found)"; fi
fi

exit $err_count
