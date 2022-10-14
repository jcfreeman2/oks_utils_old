#!/bin/sh

###########################################################################################################
####################################### Parse command line arguments ######################################
###########################################################################################################

  # script name
this=`basename $0`

  # set default values
search_pattern='*.xml'
verbose='no'
search_dir="${TDAQ_INST_PATH}/share/data"
target_dir=''
page_name='TDAQ Release Schema Files'

  # iterate command line arguments
while (test $# -gt 0)
do
  case "$1" in

    -v | --verb*)
      verbose='yes'
      ;;

    -d | --search-dir*)
      search_dir="$2" ; shift ;
      ;;

    -p | --search-pattern*)
      search_pattern="$2" ; shift ;
      ;;

    -t | --target-dir*)
      target_dir="$2" ; shift ;
      ;;

    -n | --page-name*)
      page_name="$2" ; shift ;
      ;;

    -h* | --he*)
      echo 'Usage: oks-generate-schema-docs.sh [--help] [--verbose] [--search-dir in-dir]'
      echo '                                   [--search-pattern schema-file-pattern]'
      echo '                                   --target-dir out-dir'
      echo ''
      echo 'Arguments/Options:'
      echo '   -v | --verbose             verbose output'
      echo '   -h | --help                print this message'
      echo '   -d | --search-dir in-dir   directory to search schema files'
      echo "                              current value = [${search_dir}]"
      echo '   -p | --search-pattern p    pattern for schema file names'
      echo "                              current value = [${search_pattern}]"
      echo '   -t | --target-dir out-dir  directory where to put out files'
      echo '   -n | --page-name name      provide name for generated index.html file'
      echo "                              current value = [${page_name}]"
      echo ''
      echo 'Description:'
      echo '   Creates description of found schema files. Open index.html from output dir to see generated description.'
      echo '   The description is built on fly by your browser using xsl conversion of standard oks schema xml files.'
      echo '   The above should correctly work with MS Internet Explorer 6.0, Mozilla 1.7.12 and higher versions.'
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

###########################################################################################################
################################# Prepare directory to store output files #################################
###########################################################################################################

  # test the target directory is defined via command line
if [ -z "${target_dir}" ] ; then
  echo "ERROR: target directory is not defined, existing ..."
  exit 1
fi

  # remove directory if exists
if [ -d "${target_dir}" ] ; then
  rm -rf "${target_dir}" || (echo "ERROR: cannot remove directory ${target_dir}, exiting..."; exit 1)
fi

  # remove target directory with merged schema files
target_dir2="${target_dir}/with-includes"
if [ -d "${target_dir2}" ] ; then
  rm -rf "${target_dir2}" || (echo "ERROR: cannot remove directory ${target_dir2}, exiting..."; exit 1)
fi

  # create directories
mkdir -p "${target_dir}" || (echo "ERROR: cannot create directory ${target_dir}, exiting..."; exit 1)
mkdir -p "${target_dir2}" || (echo "ERROR: cannot create directory ${target_dir2}, exiting..."; exit 1) 

###########################################################################################################
################################### Create beginning of the index file  ###################################
###########################################################################################################

index="${target_dir}/index.html"

cat <<EOF > "${index}"
<html>

<head>
<meta http-equiv="Content-Language" content="en-us">
<meta http-equiv="Content-Type" content="text/html; charset=windows-1252">
<title>${page_name}</title>
</head>

<body>

<h1 align="center">${page_name}</h1>
<table border="1" id="table1" bordercolorlight="#FFFFFF" bordercolordark="#FFFFFF" cellspacing="0" cellpadding="3">
  <tr>
    <th bgcolor="#99CCFF" align="center" rowspan="2"><font color="#000099">Schema Name</font></th>
    <th bgcolor="#99CCFF" align="center" rowspan="2"><font color="#000099">CMT Package</font></th>
    <th bgcolor="#99CCFF" align="center" colspan="3"><font color="#000099">
	Classes</font></th>
  </tr>
  <tr>
    <th bgcolor="#99CCFF" align="center"><font color="#000099">number </font></th>
    <th bgcolor="#99CCFF" align="center"><font color="#000099">direct </font></th>
    <th bgcolor="#99CCFF" align="center"><font color="#000099">all (with 
	includes)</font></th>
  </tr>
EOF

first_color='#CCCCCC'
second_color='#E5E5E5'
current_color="${first_color}"

###########################################################################################################

# set oks kernel silence

if [ "no" != "${OKS_KERNEL_SILENCE}" ] ; then
  echo 'info: run OKS in silent mode'
  OKS_KERNEL_SILENCE='yes'; export OKS_KERNEL_SILENCE
fi

for f in `find "${search_dir}" -name "${search_pattern}" -exec grep -q '<oks-schema' {} \; -print`; do
  echo " - processing file $f"
  sf=`basename ${f}`
  ff="${target_dir}/${sf}"
  sf2='&nbsp;'
  cat "${f}" | grep -v '<?xml-stylesheet' | sed 's/<oks-schema/<?xml-stylesheet type="text\/xsl" href=".\/oksSchema2Web.xsl" ?>\n\n<oks-schema/' > "${ff}" || (echo "ERROR: cannot save file as ${ff}")
  num=`cat "${f}" | grep '<file path' | wc -l | sed 's/ //g'`
  if [ "0" != "$num" ] ; then
    ff2="${target_dir2}/`basename ${f}`"
    ff2t="${ff2}.$$"
    oks_merge --out-schema-file "${ff2t}" "$f" || (echo "ERROR: oks_merge for file ${f} failed"; echo "       check DAQ/HLT-I release installation and value of TDAQ_DB_PATH")
    if [ -f "${ff2t}" ] ; then
      cat "${ff2t}" | grep -v '<?xml-stylesheet' | sed 's/<oks-schema/<?xml-stylesheet type="text\/xsl" href="..\/oksSchema2Web.xsl" ?>\n\n<oks-schema/' > "${ff2}" || (echo "ERROR: cannot save file as ${ff2}")
      rm -f "${ff2t}"
      sf2="<a href=\"with-includes/${sf}\">see info</a>"
    else
      sf2='<font color="red">merge error</font>'
    fi
  fi

  num=`grep '<class name=' ${ff} | wc -l | sed 's/ //g'`

  cmt_package_name='?'
  t=`echo ${f} | grep 'share/data' | sed "s/.*\/share\/data\///"`
  if [ ! -z "${t}" ] ;
  then
    cmt_package_name=`echo $t | sed 's/\/.*//'`
  fi

cat <<EOF2 >> "${index}"
  <tr>
    <td bgcolor="${current_color}"><font color="#000080"><b>&nbsp;`echo ${sf} | sed 's/\..*//'`</b></font>&nbsp;</td>
    <td bgcolor="${current_color}" align="right">${cmt_package_name}</td>
    <td bgcolor="${current_color}" align="right">${num}</td>
    <td bgcolor="${current_color}" align="right"><a href="${sf}">see info</a></td>
    <td bgcolor="${current_color}" align="right">${sf2}</td>
  </tr>
EOF2

  if [ "${current_color}" == "${first_color}" ] ; then current_color="${second_color}" ; else current_color="${first_color}" ; fi

done

###########################################################################################################

xsl_file_name='oksSchema2Web.xsl'
xsl_file="${TDAQ_INST_PATH}/share/data/oks/xsl/${xsl_file_name}"

if [ ! -f "${xsl_file}" ] ; then
  echo "ERROR: cannot find oks schema xsl file '${xsl_file}';"
  echo '       check intallation or setup of DAQ/HLT-I release and value of TDAQ_INST_PATH variable, exiting...' 
  exit 1
fi

cp "${xsl_file}" "${target_dir}" || (echo "ERROR: cannot copy file ${xsl_file} to directory ${target_dir}, exiting..."; exit 1)

###########################################################################################################


cat <<EOF >> "${index}"
</table>
<p><i><font size="2">This page was automatically generated by $this "`date`" on "`uname -n`"</font></i><br>
&nbsp;</p>

</body>

</html>
EOF

echo "The oks schema files with xsl description have been generated. See file ${index}."
