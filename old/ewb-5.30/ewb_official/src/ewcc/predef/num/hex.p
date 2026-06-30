sub PD_hex($)
{ my $code=$_[0];
  my $res;
  $code=substr($code, 3);

#gestione parentesi 1/2
  my $hp=0;
  if (substr($code,0,1) eq "(")
  { my $rr; ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.= "\$ax=(hex \$ax);$T";

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "exec: manca ).";
    }
    my $rr; ($code,$rr)=delperc(substr($code,1));
  }

  return ($code,$res);
}
