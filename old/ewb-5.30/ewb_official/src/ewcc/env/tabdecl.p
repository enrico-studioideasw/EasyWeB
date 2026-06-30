#***
sub tabdecl($)
{ #Qui ho una dichiarazione di tabella.. mi serve solo per l' ambiente.
  #Ricordo che ora il prototipo della funzione variable e' diverso.
  my $code=$_[0];
  my $res;
  $code=substr($code, 6);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $name;
  my @values=();
  ($code, $name)=variable($code);
  checkpredef $name;
  #$code=substr($code, length($name));
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code, 0, 1) ne "=") {  errore "table: mi aspettavo \"=\". "; }
  $code=substr($code,1);
  my ($rf,$rg)=delperc($code);
  $code=",".$rf;
  #variable e' un token elementare: non taglia il code e lo devo fare io a mano.

  while (substr($code, 0,1) eq ",")
  { $code=substr($code, 1);
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
    ($code, $values[$#values+1])=variable($code);
    checkpredef $values[$#values];
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
  if (substr($code, 0, 1) ne ";") { errore "manca ; a fine riga."; };
  $tables[$#tables+1]=$name.":".join(":",@values);
  $res.="\tmy \$TB_$name=\"".$name.":".join(":",@values)."\";$T";
  #ogni elemento di una tabella deve diventare una variabile.
  #questo se la variabile non esiste o e' gia' dichiarata da una tabella.
  my $v;
  foreach $v(@values)
  { my $toadd=1;
    my $k; foreach $k(@variables)
    { if ($k eq $v.":v")
      { $toadd=0; }
    }
    if ($toadd)
    { $res=$res. "my \$EWB_".$v."=\"\"; $T";
      $variables[$#variables+1]= $v.":t";
    }
  }
  return ($code,$res);
}
