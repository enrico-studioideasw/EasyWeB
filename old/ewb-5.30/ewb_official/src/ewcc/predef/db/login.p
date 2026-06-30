sub PD_login($)
{ my $code=$_[0];
  my $hp=0;
  my $name;
  my $rr;
  ($code,$rr)=delperc(substr($code, 5));
  if (substr($code, 0,1) eq "(")
  { ($code,$rr)=delperc(substr($code, 1));
    $hp=1;
  }
  my $res= "";#(ewb_login(";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res=$res."\$ax=ewb_login(\$ax);$T";
  if (($hp) && (substr($code, 0, 1) ne ")"))
  { errore "login - parentesi non chiusa.";
  }
  if ($hp) { ($code,$rr)=delperc(substr($code, 1)); }
  return ($code,$res);
}
