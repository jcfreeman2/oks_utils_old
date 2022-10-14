#!/bin/sh

########################################################################

echo "CHECKING ALLOC and MEMORY-ALLOCATION-MUTEX MECHANISMS..."

if ${1}/oks_utils_alloc_test
then
  echo "alloc check passed";
else
  echo "OKS Error: alloc check failed"
  exit 1
fi

########################################################################

schema_file="/tmp/oks_tutorial.schema.$$.xml"
data_file="/tmp/oks_tutorial.data.$$.xml"

########################################################################

echo "CHECKING OKS LIBRARY..."

if ${1}/oks_tutorial ${schema_file} ${data_file}
then
  echo "oks_tutorial check passed";
else
  echo "OKS Error: check failed with oks_tutorial"
  exit 1
fi

if oks_dump ${schema_file} ${data_file}
then
  echo "oks_dump check passed"
else
  echo "OKS Error: check failed with oks_dump"
  exit 1
fi

rm -f ${schema_file} ${data_file}

echo "DONE CHECKING OKS LIBRARY..."

########################################################################
