sub PD_show($)
{ my $code=$_[0];
  my $res="";
  my $rr;
  ($code,$rr)=delperc $code;
  $res= "";#print join((";
  $code=substr($code, 4);
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.= "\$ax=print join((\$ax), split(\"<>\",\$EWB_template));$T";
  return ($code, $res);
}
