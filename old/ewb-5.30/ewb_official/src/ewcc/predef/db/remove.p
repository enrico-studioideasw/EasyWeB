sub PD_remove($)
{ my $code=$_[0];
  my $hp=0;
  my $name;
  my $rr;
  ($code,$rr)=delperc(substr($code, 6));
  if (substr($code, 0,1) eq "(")
  { ($code,$rr)=delperc(substr($code, 1));
    $hp=1;
  }
  my $res= "";#ewb_remove(";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\$ax=ewb_remove(\$ax);$T";

  if (($hp) && (substr($code, 0, 1) ne ")"))
  { errore "remove - parentesi non chiusa.";
  }
  if ($hp) { ($code,$rr)=delperc(substr($code, 1)); };
  return ($code,$res);
}
