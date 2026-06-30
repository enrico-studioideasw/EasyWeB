sub PD_return($)
{ my $code=$_[0];
  #my $res= "(return (";
  $code=substr($code, 6);
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  #$res.= "))";
  return ($code,$res);
}
