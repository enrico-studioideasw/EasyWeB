sub PD_exist($)
{ my $code=$_[0];
  my $hp=0;
  my $name;
  my $rr;
  ($code,$rr)=delperc(substr($code, 5));
  if (substr($code, 0,1) eq "(")
  { ($code,$rr)=delperc(substr($code, 1));
    $hp=1;
  }
  my $res= "";#(ewb_exist(";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\$ax=ewb_exist(\$ax);$T";



  if ($hp) { ($code,$rr)=delperc(substr($code, 1)); }
  return ($code,$res);
}
