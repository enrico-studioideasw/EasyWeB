sub PD_add($)
{ my $code=$_[0];
  my $hp=0;
  my $name;
  my $res="";
  my $rr;
  my ($code,$rr)=delperc(substr($code, 3));
  if (substr($code, 0,1) eq "(")
  { ($code,$rr)=delperc(substr($code, 1));
    $hp=1;
  }

  my $rb;
  ($code,$rb)=calcexp($code);
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if (($hp) && (substr($code, 0, 1) ne ")"))
  { errore "add - parentesi non chiusa.";
  }

  $res.= "\$ax=ewb_add(\$ax);$T";
  if ($hp) { ($code,$rr)=delperc(substr($code, 1)); }
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  return ($code, $res);
}
