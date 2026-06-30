sub sqlrun($)
{ #deve eseguire la query e tornare l' eventuale risultato in array.
  #non ho necessità di tornare gli eventuali errori.. o meglio avrei
  #problemi a gestirli. Ad esempio posso ignorare un errore dovuto ad una
  #doppia create, me non quelli dovuti all' assenza del dbase.
  if (index($_[0], "CREATE DATABASE")<0)
  { $sql="use $EWB_sqldbase;".$_[0].";"; } else { $sql=$_[0].";"; };
  if ($EWB_sqlserver ne "")
  { $sql="CONNECT $ewb_sqlserver;".$sql;
  };
  my $r= qx{mysql -e \"$sql\"};
  $r=join("#", split("\t", $r));
  my @a=split("\n", $r);
  $a[0]="";
  $r=join("\n", @a);
  $r=substr($r, 1, length($r));
  @a=split("\n", $r);
  return @a;
};
