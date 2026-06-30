sub PD_abs($)
{ my $code=$_[0];
  #my $res= "(abs (";
  $code=substr($code, 3);

#gestione parentesi 1/2
  my $hp=0;
  if (substr($code,0,1) eq "(")
  { my $t;($code,$t)=delperc(substr($code,1));
    $hp=1;
  }

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res=$res. "\$ax=abs(\$ax);$T";

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "exec: manca ).";
    }
    my $t;($code,$t)=delperc(substr($code,1));
  }

  return ($code,$res);
}
