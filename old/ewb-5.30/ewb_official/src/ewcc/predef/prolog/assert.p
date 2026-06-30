sub PD_assert($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $a;
  ($code, $a)=variable($code);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $hp;
  $res.= "";#"(EWassert(";
  if (substr($code, 0,1) eq "(")
  { ($code,$rcd)=delperc(substr($code, 1));
    $hp=1;
  }
#qui una variabile anziche' il codice.
  ($code, $a)=variable($code);
  checkpredef($a);
  #$res.= "\"".$a."\"";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if ((substr($code, 0, 1) ne ","))
  { errore "assert - manca un parametro.";
  }
  ($code,$rcd)=delperc(substr($code,1));
  #$res.= ",";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.="\$ax=EWassert(\"$a\",\$ax);$T";

  if (($hp) && (substr($code, 0, 1) ne ")"))
  { errore "assert - parentesi non chiusa.";
  }
  if ($hp) { ($code,$rcd)=delperc(substr($code, 1)); }
  return ($code,$res);
}
