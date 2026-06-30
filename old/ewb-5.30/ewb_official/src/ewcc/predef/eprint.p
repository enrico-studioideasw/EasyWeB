sub PD_eprint($)
{ my $code=$_[0];
  my $res = "";
  my $t;($code,$t)=delperc $code;

  $code=substr($code, 6);
  my $hp=0;
  if(substr($code,0,1) eq "(")
  { my $t; ($code,$t)=delperc(substr($code,1)); $hp=1;
  }
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  $res=$res. "printf stderr \$ax;$T";
my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if ($hp)
  { if(substr($code,0,1) ne ")") { errore "eprint - manca chiusa parentesi."; };
    my $t; ($code,$t)=delperc(substr($code,1));
  };

  return ($code,$res);
}
