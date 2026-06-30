sub PD_print($)
{ my $code=$_[0];
  my $res="";
  my $t; ($code,$t)=delperc $code;

  $code=substr($code, 5);
  my $hp=0;
  if(substr($code,0,1) eq "(")
  { my $t; ($code,$t)=delperc(substr($code,1)); $hp=1;
  }
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if ($hp)
  { if(substr($code,0,1) ne ")") { errore "print - manca chiusa parentesi."; };
    my $t;($code,$t)=delperc(substr($code,1));
  };
  $res=$res. "print \$ax;$T";
  return ($code,$res);
}
