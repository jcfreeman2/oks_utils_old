#!/usr/bin/perl

  use CGI qw/:standard/;

  $tdaq_release='tdaq-02-00-00';

  @DB = (
    { name => "TDAQ Development",  connect_as => "oracle://devdb10/tdaq_dev_backup", db_owner => "onlcool" },
    { name => "Point-1 (offline)", connect_as => "oracle://atlas_oks/r",             db_owner => "atlas_oks_archive" },
    { name => "Point-1 (online)",  connect_as => "oracle://atonr_oks/r",             db_owner => "atlas_oks_archive" },
  );

  @INTERVALS = ( '*','1 hour','2 hours','6 hours','12 hours',
                 '1 day','2 days','1 week','2 weeks',
                 '1 month','2 months','6 months','1 year','2 years');

  @DBNAMES = ('');
  @VERSIONS = ('');
  @PARTITIONS = ('');
  @RELEASE_NAMES = ('*');


  for $i ( 0 .. $#DB ) {
    $v=$DB[$i]{"name"};
    push @DBNAMES, $v;
  }

  $db_name='';
  $connect_as='';
  $db_owner='';
  $show_release='';
  $show_user='';
  $show_host='';
  $show_size='';
  $show_description='';
  $show_incremental_versions='-b';

 ##################################################################################################################

   # set table config cookies

  %table_columns_c = cookie('OksArchive:table_columns');
  %table_details_c = cookie('OksArchive:table_details');
  %table_sort_by_c = cookie('OksArchive:table_sort_by');

  @TABLE_COLUMNS = ('release','user','host','size','description');  # checkbox_group(-name=>'table_columns')
  @TABLE_DETAILS = ('incremental versions','usage');                # checkbox_group(-name=>'ls_details')
  @TABLE_SORT_BY = ('srt1','srt2','srt3','srt4');                   # 4 menues

   # if the check boxes of TABLE COLUMNS (checkbox_group(-name=>'table_columns')) are set, overwrite cookie values
  if(param('table_columns')) {
    foreach (@TABLE_COLUMNS) { $table_columns_c{$_} = 'no'; }
    @tbl_d=param('table_columns');
    for $i ( 0 .. $#tbl_d ) { $d=$tbl_d[$i]; $table_columns_c{$d}='yes'; }
  }
  foreach (@TABLE_COLUMNS) { $table_columns_c{$_} = $table_columns_c{$_} || 'no'; }

   # if the check boxes of TABLE DETAILS (checkbox_group(-name=>'ls_details')) are set, overwrite cookie values
  if(param('ls_details')) {
    foreach (@TABLE_DETAILS) { $table_details_c{$_} = 'no'; }
    @tbl_d=param('ls_details');
    for $i ( 0 .. $#tbl_d ) { $d=$tbl_d[$i]; $table_details_c{$d}='yes'; }
  }
  foreach (@TABLE_DETAILS) { $table_details_c{$_} = $table_details_c{$_} || 'yes'; }

   # if sort by are set, overwrite cookie values
  foreach (@TABLE_SORT_BY) {
    if (param("$_")) { $table_sort_by_c{$_} = param("$_"); }
    else { $table_sort_by_c{$_} = $table_sort_by_c{$_} || ' '; }
  }

  $tz_name = param('tmz') || cookie('OksArchive:time-zone') || 'UTC';

  push @COOKIES, cookie(-name=>'OksArchive:time-zone',     -value=>"$tz_name",        -expires=>'+180d');
  push @COOKIES, cookie(-name=>'OksArchive:table_details', -value=>\%table_details_c, -expires=>'+180d');
  push @COOKIES, cookie(-name=>'OksArchive:table_columns', -value=>\%table_columns_c, -expires=>'+180d');
  push @COOKIES, cookie(-name=>'OksArchive:table_sort_by', -value=>\%table_sort_by_c, -expires=>'+180d');

 ##################################################################################################################

  print header(-cookie=>\@COOKIES);

 ##################################################################################################################

  @TIME_ZONE_NAMES = ('UTC');
  push @TIME_ZONE_NAMES, 'CERN';

  @result = `./rn_table.sh $tdaq_release foo bar -z list-regions`;
  $status=$? >> 8;

  if($status != 0) {
    for $i ( 0 .. $#result ) {
      print span({-style=>'color: red;'},$result[$i],"<br>");
    }
    return 0;
  }

  for $i ( 0 .. $#result ) {
    $z=$result[$i];
    $z =~ s/^\s*(.*?)\s*$/$1/;
    push @TIME_ZONE_NAMES,"$z";
  }


  if(param()) {

      # test if the compare versions function has been choosen

    $v1=param('v1');

    if($v1 ne "") {
      $v2=param('v2');
      $pts=param('pts');
      &compare_data_versions();
      return 0;
    }


      # test if the database has been choosen

    $db_name=param('db');

    if($db_name eq "") {
      $db_name=param('dbh');
    }

    if($db_name eq "") {
      print
        start_html('ATLAS OKS Archive'),
        start_form,
        h1('ATLAS OKS Archive'),
        "Select database: ", popup_menu(-name=>'db', -values=> [ @DBNAMES ], -default=>'' ), p, submit, end_form, hr,
        "ERROR: Select a database!", hr;
      &sign();
      return 0;
    }
  }

  if("$db_name" eq "") {
    print
      start_html('ATLAS OKS Archive'),
      start_form,
      h1('ATLAS OKS Archive'),
      "Select database: ", popup_menu(-name=>'db', -values=> [ @DBNAMES ], -default=>'' ), p, submit, end_form, hr;

    &sign();
    return 0;
  }
  else {
    print
      start_html('ATLAS OKS Archive'),
      start_form,
      h1("ATLAS OKS Archive for \"$db_name\" database"),
      hidden(-name=>'dbh', -default=> [$db_name]);
  }

  for $i ( 0 .. $#DB ) {
    $v=$DB[$i]{"name"};
    if ($db_name eq "$v") {
      $db_name=$v;
      $connect_as=$DB[$i]{"connect_as"};
      $db_owner=$DB[$i]{"db_owner"};
    }
  }

  $got_releases=0;

  @RELEASE_NAMES=param('hn');

  if($#RELEASE_NAMES < 1) {
    push @RELEASE_NAMES, '*';
#    @result = `./vtable.sh $tdaq_release $connect_as $db_owner -l $show_incremental_versions $show_description`;
    @result = `./vtable.sh $tdaq_release $connect_as $db_owner -l`;
    $status=$? >> 8;

    if($status != 0) {
      for $i ( 0 .. $#result ) {
        print span({-style=>'color: red;'},$result[$i],"<br>");
      }
      return 0;
    }

    for $i ( 0 .. $#result ) {
      if ($result[$i] =~ /^Found .*/) { next; }
      $result[$i] =~ s/ - //;
      $result[$i] =~ s/\'//g;
      $result[$i] =~ s/^\s*(.*?)\s*$/$1/;
      push @RELEASE_NAMES, $result[$i];
    }

    $got_releases=1;

    @DN = ();

    for $i ( 0 .. $#DURATIONS ) {
      $v=$DURATIONS[$i]{"name"};
      push @DN, $v;
    }

    print
      "Show configurations archived between now and ", popup_menu(-name=>'time_int', -values=> [ @INTERVALS ] ), " ago", p;
  }

  print
    "Select release name: ", popup_menu(-name=>'names', -values=> [ @RELEASE_NAMES ] ), p,
    hidden(-name=>'hn', -default=> [ @RELEASE_NAMES ]);

  if($got_releases == 1) {
    print submit, end_form, hr;
    &sign();
    return 0;
  }

  $t=param('time_int');
  $past='';

  if ("$t" ne "") {
    $t =~ s/^\s*(.*?)\s*$/$1/;

    if("$t" ne "*") {
      chomp($past = `date -u '+%F %T' --date="$t ago"`);
    }

    $show_incremental_versions='';
  }

  @SORT = (
    { name => "version",               option => "v" },
    { name => 'time',                  option => 't' },
    { name => 'user name',             option => 'u' },
    { name => 'host name',             option => 'h' },
    { name => 'partition name',        option => 'p' },
    { name => 'version (desc)',        option => 'V' },
    { name => 'time (desc)',           option => 'T' },
    { name => 'user name (desc)',      option => 'U' },
    { name => 'host name (desc)',      option => 'H' },
    { name => 'partition name (desc)', option => 'P' }
  );

  @SORT_BY = ( ' ' );

  for $i ( 0 .. $#SORT) {
    $v=$SORT[$i]{"name"};
    push @SORT_BY, $v;
  }

  if("$tz_name" eq "" || "$tz_name" eq "UTC") { $tz_table_title = 'UTC';              $cmd_line= '';                 $tz_pfx='';            }
  elsif("$tz_name" eq "CERN")                 { $tz_table_title = 'CERN local time';  $cmd_line= '-Z Europe/Zurich'; $tz_pfx=' local time'; }
  else                                        { $tz_table_title = 'local time';       $cmd_line= "-Z $tz_name";      $tz_pfx=' local time'; }

  print
    'Show configurations archived from ', textfield('archived_from',$past,19,20), ' till ', textfield('archived_till','',19,20), " $tz_name$tz_pfx",
    '<br>&nbsp;&nbsp;<font size=-1><i>(leave empty to be ignored or use <a href="http://www.iso.org/iso/en/prods-services/popstds/datesandtime.html">ISO 8601</a> date-time format to provide a value)</i></font>', p,
    'Show user ', textfield('user','',12), ' host ', textfield('host','',16), ' partition ', textfield('partition','',20),
    '<br>&nbsp;&nbsp;<font size=-1><i>(leave a field empty to be ignored, or put exact name, or use expression with <a href="#wildcards">wildcards</a>)</i></font>', p,
    h3('User preferences'), p,
    'Select timezone: ', popup_menu(-name=>'tmz', -values=>\@TIME_ZONE_NAMES, -default=>"$tz_name" ), p,
    "Show: ";

  if ($table_details_c{'usage'} eq "yes") { $cmd_line="$cmd_line -u"; }
  if ($table_details_c{'incremental versions'} eq "yes") { $show_incremental_versions=''; }

  @tabel_details_selected = ();
  foreach (@TABLE_DETAILS) {
    if($table_details_c{$_} eq 'yes') { push @tabel_details_selected, "$_"; }
  }

  print
     checkbox_group(-name=>'ls_details', -values=>[@TABLE_DETAILS], -defaults=>[@tabel_details_selected]), p,
    'Select optional table columns: ';

  $show_release=$table_columns_c{'release'};
  $show_description=$table_columns_c{'description'};
  $show_user=$table_columns_c{'user'};
  $show_host=$table_columns_c{'host'};
  if ($table_columns_c{'size'} eq "yes") { $show_size='-z'; }

  @tabel_columns_selected = ();
  foreach (@TABLE_COLUMNS) {
    if($table_columns_c{$_} eq 'yes') { push @tabel_columns_selected, "$_"; }
  }

  print
    checkbox_group(-name=>'table_columns', -values=>[@TABLE_COLUMNS], -defaults=>[@tabel_columns_selected]), p,
    "Sort result by ";

  foreach (@TABLE_SORT_BY) {
    print popup_menu(-name=>"$_", -values=> [ @SORT_BY  ], -default=>"$table_sort_by_c{$_}" ), ' ';
  }

  print p,
    submit,
    end_form,
    hr;

  if (param()) {
     $cmd_line="$cmd_line -d";

     if("$past" eq "") {
       $x=param('archived_from'); if ("$x" ne "")  { $x =~ s/ /T/g; $cmd_line="$cmd_line -S $x"; }
       $x=param('archived_till'); if ("$x" ne "")  { $x =~ s/ /T/g; $cmd_line="$cmd_line -T $x"; }
     }
     else {
       $past =~ s/ /T/g;
       $cmd_line="$cmd_line -S $past";
     }

     $x=param('names');

     if ("$x" ne "*") {
       $cmd_line="$cmd_line -r $x"
     }

     $x=param('host');

     if ("$x" ne "") {
       $cmd_line="$cmd_line -o $x"
     }

     $x=param('user');

     if ("$x" ne "") {
       $cmd_line="$cmd_line -e $x"
     }

     $x=param('partition');

     if ("$x" ne "") {
       $cmd_line="$cmd_line -p $x"
     }

     $sopt='';
     foreach (@TABLE_SORT_BY) {
       $x=$table_sort_by_c{$_};
       if ("$x" ne ' ') {
         for $i ( 0 .. $#SORT ) {
           $v=$SORT[$i]{"name"};
           if ($x eq "$v") {
             $y=$SORT[$i]{"option"};
             $sopt="$sopt $y";
           }
         }
       }
     }

     if ("$sopt" ne "") {
       $sopt =~ s/ //g;
       $cmd_line="$cmd_line -t $sopt";
     }

     print h2('Archived Versions');

     @result = `./vtable.sh $tdaq_release $connect_as $db_owner $show_size $show_incremental_versions $cmd_line`;
     $status=$? >> 8;

     if($status != 0) {
       for $i ( 0 .. $#result ) {
         print span({-style=>'color: red;'},$result[$i],"<br>");
       }
       return 0;
     }

     $count_base_versions=0;
     $count_inc_versions=0;
     $count_used=0;

     $table_title_bg_color="#99CCFF";
     $table_base_bg_color="#FFFF99";
     $table_inc_bg_color="#C0C0C0";
     $table_usage_bg_color="#CCFFCC";

     print "<table border=1 style=\"border-collapse: collapse\">",
           "<tr>",
	   "<th nowrap bgcolor=\"$table_title_bg_color\">Version</th>";

     if($show_release eq 'yes') {
       print "<th nowrap bgcolor=\"$table_title_bg_color\">Release</th>";
     }

     print "<th nowrap bgcolor=\"$table_title_bg_color\">Date ($tz_table_title)</th>";

     if($show_user eq 'yes') {
       print "<th nowrap bgcolor=\"$table_title_bg_color\">User</th>";
     }

     if($show_host eq 'yes') {
       print "<th nowrap bgcolor=\"$table_title_bg_color\">Host</th>";
     }

     if($show_size eq '-z') {
       print "<th nowrap bgcolor=\"$table_title_bg_color\">Size</th>";
     }

     if($show_description eq 'yes') {
       print "<th nowrap bgcolor=\"$table_title_bg_color\">Description</th>";
     }

     print "</tr>";

     for $i ( 0 .. $#result ) {
       if ($result[$i] =~ /^=.*/) {
         next;
       }
       ($x,$version, $release, $date, $user, $host, $size, $description) = split(/\|/,$result[$i]);

       $version =~ s/ //g;
       $release =~ s/ //g;
       $date =~ s/^\s*(.*?)\s*$/$1/;
       $user =~ s/ //g;
       $host =~ s/ //g;
       $description =~ s/^\s*(.*?)\s*$/$1/;

       if ($version eq "Version") { next; }   # skip table's caption

       $bg_color="white";
       if ($version eq "" ) {
         $bg_color=$table_usage_bg_color;
         $count_used=$count_used+1;
       }
       elsif ($version =~ /[0-9]*\.[0-9]*\.[0-9]*/) {
         push @VERSIONS, $version;
         $bg_color=$table_inc_bg_color;
         $count_inc_versions=$count_inc_versions+1;
	 ($sv, $dv, $bv) = split(/\./,$version);
	 $partition = '';
	 if ( $description =~ /^oks2coral: partition .*/ ) {
	   $partition = $description;
	   $partition =~ s/oks2coral: partition\s(.*?)\s\(.*\)/$1/;
	   $vfound='no';
	   for $j ( 0 .. $#PARTITIONS ) {
	     if ($PARTITIONS[$j] eq $partition) {
	       $vfound='yes';
	       last;
	     }
	   }

	   if($vfound eq 'no') {
             push @PARTITIONS, $partition;
	   }
	 }
	 $version = "<a href=\"getdata.sh?$tdaq_release&$connect_as&$db_owner&$sv&$dv&$partition\">$version</a>"
       }
       elsif ($version =~ /[0-9]*\.[0-9]*/) {
         push @VERSIONS, $version;
         $bg_color=$table_base_bg_color;
         $count_base_versions=$count_base_versions+1;
	 ($sv, $dv) = split(/\./,$version);
	 $version = "<a href=\"getdata.sh?$tdaq_release&$connect_as&$db_owner&$sv&$dv&\">$version</a>"
       }

       print
         "<tr>",
          "<td bgcolor=\"$bg_color\">$version</td>";

       if($show_release eq 'yes') {
         print "<td bgcolor=\"$bg_color\">$release</td>";
       }

       print "<td bgcolor=\"$bg_color\">$date</td>";

       if($show_user eq 'yes') {
	 print "<td bgcolor=\"$bg_color\">$user</td>";
       }

       if($show_host eq 'yes') {
	 print "<td bgcolor=\"$bg_color\">$host</td>";
       }

       if($show_size eq '-z') {
         $size =~ s/ //g;
         print "<td bgcolor=\"$bg_color\" align=\"right\">$size</td>";
       }

       if($show_description eq 'yes') {
         print "<td bgcolor=\"$bg_color\">$description</td>";
       }

       print "</tr>";
     }

    print "</table>", p;

    print "Selected $count_base_versions base";

    if($show_incremental_versions eq '') {
      print " and $count_inc_versions incremental"
    }

    print " version(s)";

    if($count_used ne 0) {
      print " used by $count_used run(s)";
    }

    print '.', p, hr;


    if($#VERSIONS > 1) {
      print h2('Compare Versions'),
            start_form,
	    "Select two versions to see differences between them: ",
            popup_menu(-name=>'v1',  -values=> [ @VERSIONS   ], -default=>'' )," ",
            popup_menu(-name=>'v2',  -values=> [ @VERSIONS   ], -default=>'' ),p,
	    "Optionally select a partition to see differences between it's objects: ",
	    popup_menu(-name=>'pts', -values=> [ @PARTITIONS ], -default=>'' )," ",
            hidden(-name=>'info', -default=>[$db_name,$connect_as,$db_owner]),
	    " (if selected, it must exist in both versions!)", p,
 	    submit,
            end_form,
            hr;
    }

    print h2('Help'),
        h3('Table Colors and Versions Legend'),
        "<table border=\"1\" style=\"border-collapse: collapse\">",
	"<tr><th colspan=\"2\" bgcolor=\"$table_title_bg_color\" nowrap><b>Table colors / Versions</b></th></tr>",
	"<tr><td bgcolor=\"$table_base_bg_color\">Base version </td>",
	  "<td bgcolor=\"$table_base_bg_color\">(version format: schema-version.data-version),e.g.:<br>",
	  "1.12 - base version with schema-version = 1 and data-version = 12 </td></tr>",
	"<tr><td bgcolor=\"$table_inc_bg_color\">Incremental version</td>",
	  "<td bgcolor=\"$table_inc_bg_color\">(version format: schema-version.data-version.used-based-version), e.g.:<br>",
	  "1.34.12 - incremental version with data-version = 34 built on top of base version 1.12 </td></tr>",
	"<tr><td bgcolor=\"$table_usage_bg_color\">Usage of versions </td>",
	  "<td bgcolor=\"$table_usage_bg_color\">(description contains partition and run number)</td></tr></table>",
	h3('Size Column'),
	"The size of a base version is numbers of rows in relational tables to describe it's data. ",
	"For an incremental version the size is numbers of additional rows to store differences from it's base ",
	"version. A size is presented in form O:A:R, where: O is a number of objects table rows, A is number of ",
	"attribute value table rows and R is a number of relationship value table rows.",
	h3('Download XML Files'),
	"Click on a base or incremental version from table. After few moments \"File Download\" dialog appears, which points to ",
	"compressed schema and data OKS XML files. Save both files into same directory to be able to use them with OKS tools. ",
        "When downloaded, a base version contains configuration objects of all partitions from database repositories; ",
        "an incremental version contains only configuration objects of partition, for which it has been created.",
        h3('Expression with wildcards'),
        "<a name=\"wildcards\"></a>To provide a name one can use expression with wildcards, where:",
        "<blockquote><table border=\"0\">",
          "<tr> <td><b>%</b> (i.e. percent symbol)</td> <td>- replaces any string of zero or more characters</td> </tr>",
	  "<tr> <td><b>_</b> (i.e. underscore symbol)</td> <td>- replaces any single character</td> </tr>",
        "</table></blockquote>",
	hr;

  }

  &sign();

sub compare_data_versions {
  @info=param('info');
  if ( $pts eq "" ) {
    $title="Compare data versions $v1 and $v2 from $info[0] database";
  }
  else {
    $title="Compare objects of partition $pts in data versions $v1 and $v2 from $info[0] database";
  }
  print
    start_html("ATLAS OKS Archive: $title"),
    h1($title);

  $count=0;

  @result = `./cmp_data_versions.sh $tdaq_release $info[1] $info[2] $v1 $v2 $pts`;
  $status=$? >> 8;
  #print "Finished with status: ",$status,"<br>";

  if($status != 0) {
    for $i ( 0 .. $#result ) {
      print span({-style=>'color: red;'},$result[$i],"<br>");
    }
    return 0;
  }
  
  $table_title_bg_color="#99CCFF";
  $table_object_bg_color="#FFFF99";
  $table_obj_exist_bg_color="#CCFF99";
  $table_no_obj_bg_color="#C0C0C0";
  $table_attr_bg_color="#CCFFCC";
  $table_rel_bg_color="#FFE1FF";

  print "<table border=1 style=\"border-collapse: collapse\">",
         "<tr>",
	  "<th colspan=\"2\" nowrap bgcolor=\"$table_title_bg_color\">&nbsp;</th>",
	  "<th nowrap bgcolor=\"$table_title_bg_color\">Version $v1</th>",
	  "<th nowrap bgcolor=\"$table_title_bg_color\">Version $v2</th>",
	 "</tr>";

  $ar_color='';       # color variable shared between attributes and relationships
  $ar1_printed='no';

  for $i ( 0 .. $#result ) {
    $d = $result[$i];
    $d =~ s/^\s*(.*?)\s*$/$1/;      # trim spaces

    if ($d eq "") { next; }         # skip empty lines

    elsif ($d =~ /^DIFFERENCE .* The OBJECTS .* differed:/) {
      ($x, $object_name, $y) = split(/\"/,$d);
      print "<tr><td colspan=\"4\" bgcolor=\"$table_object_bg_color\"><b>$object_name</b></td></tr>";
      $count=$count+1;
    }

    elsif ($d =~ /^DIFFERENCE .* There is no object/) {
      ($x, $object_name, $y, $file, $z) = split(/\"/,$d);

      if($file eq "$v2.data.xml") {
        $c1=$table_obj_exist_bg_color;
        $c2=$table_no_obj_bg_color;
        $t1='exists';
        $t2='does not exist';
      }
      else {
        $c2=$table_obj_exist_bg_color;
        $c1=$table_no_obj_bg_color;
        $t2='exists';
        $t1='does not exist';
      }

      print "<tr>",
             "<td colspan=\"2\" bgcolor=\"$table_object_bg_color\"><b>$object_name</b></td>",
	     "<td bgcolor=\"$c1\">$t1</td>",
	     "<td bgcolor=\"$c2\">$t2</td>",
	    "</tr>";
      $count=$count+1;
    }

    elsif ($d =~ /^[0-9]*.* The RELATIONSHIP .* differed:/) {
      ($x, $rel, $y) = split(/\"/,$d);
      $ar_color=$table_rel_bg_color;
      print "<tr><td bgcolor=\"$ar_color\">&nbsp;</td><td bgcolor=\"$ar_color\"><i>$rel</i></td>";
      $ar1_printed='no';
    }

    elsif ($d =~ /^[0-9]*.* The ATTRIBUTE .* differed:/) {
      ($x, $rel, $y) = split(/\"/,$d);
      $ar_color=$table_attr_bg_color;
      print "<tr><td bgcolor=\"$ar_color\">&nbsp;</td><td bgcolor=\"$ar_color\"><i>$rel</i></td>";
      $ar1_printed='no';
    }

    elsif ($d =~ /^the value in the FILE.*/) {
      $d =~ s/.* is: //;
      print "<td bgcolor=\"$ar_color\" valign=\"top\">$d</td>";
      if    ($ar1_printed eq 'no')  { $ar1_printed = 'yes'; }
      elsif ($ar1_printed eq 'yes') { $ar1_printed = 'no'; print "</tr>";}
    }

    #print $result[$i];
  }

  print "</table>",p;
  

  $out='';

  if ( $count == 0 ) {
    $out="No differences found.";
  }
  elsif ($count == 1) {
    $out="1 object is different.";
  }
  else {
    $out="$count objects are different.";
  }

  print span({-style=>'color: darkblue;'},$out,p);

  &sign();
}

sub sign {
  $now_string = localtime;
  print "<i><font size=-1>Generated automatically at $now_string. ",
        "Report bugs to <a href=\"http://consult.cern.ch/xwho/people/432778\">author</a>.",
        "</font></i></body></html>\n";
}
