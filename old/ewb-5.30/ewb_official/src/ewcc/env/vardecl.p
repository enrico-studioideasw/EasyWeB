#***
sub vardecl($)
{ my $code=$_[0];
  my $res="";
  $code=substr($code, 3);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  my $first=0;
  do
  { if ($first) { my $t; ($code,$t)=delperc(substr($code, 1)); }
    $first=1;
    $res=$res. "\nmy \$";
    my $name;
    ($code, $name)=variable($code);
    checkpredef $name;
    $res=$res. "EWB_".$name;
    $res=$res. "; if (\$ISASKTO) { \$EWB_$name = get_par(\"$name\"); };$T";
    #aggiungo la variabile in ambiente.
      $variables[$#variables+1]= $name.":v";
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
  while ( substr($code, 0, 1) eq  ",");
  if (substr($code, 0, 1) ne ";")
  { errore "manca ; dopo dichiarazione di variabili.";
  }
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  return ($code,$res);
}
