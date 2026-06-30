sub sqlrun($)
{ #deve eseguire la query e tornare l' eventuale risultato in array.
  #non ho necessità di tornare gli eventuali errori.. o meglio avrei
  #problemi a gestirli. Ad esempio posso ignorare un errore dovuto ad una
  #doppia create, me non quelli dovuti all' assenza del dbase.
  #unica differenza tra questa libreria e quella di gestione di mysql.


$db = $EWB_sqldbase;     	# your username (= login name  = account name )
$host = $EWB_sqlserver;   	# = "localhost", the server your are on.
$user = "easyweb";       	# your Database name is the same as your account name.
$pwd = "";               	# Your account password


# connect to the database.


$dbh = DBI->connect( "DBI:mysql:$db:$host", $user, $pwd)
                or die "Connecting : $DBI::errstr\n ";

  if (index($_[0], "CREATE DATABASE")<0)
  { $sql="use $EWB_sqldbase;".$_[0].";"; } else { $sql=$_[0].";"; };
  if ($EWB_sqlserver ne "")
  { $sql="CONNECT $EWB_sqlserver;".$sql; 
  };

$sth = $dbh->prepare($sql) or die "preparing: ",$dbh->errstr;
$sth->execute or die "executing: ", $dbh->errstr;

#questa e' da modificare. Vanno comunque gestiti i nomi dei campi.. non facile.
while ($row = $sth->fetchrow_hashref)           
{
        print $row->{'some_field_name'},'<BR>'; 
}

#  my $r= qx{mysql -e \"$sql\"};
#  $r=join("#", split("\t", $r));
#  my @a=split("\n", $r);
#  $a[0]="";
#  $r=join("\n", @a);
#  $r=substr($r, 1, length($r));
#  @a=split("\n", $r);
#  return @a;
};
