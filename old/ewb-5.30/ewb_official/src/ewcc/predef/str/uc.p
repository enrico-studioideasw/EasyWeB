sub PD_uc($)
{ my $code=$_[0];
  my $res=""; #= "(uc (";
  my $rr; ($code,$rr)=delperc(substr($code, 2));

#gestione parentesi 1/2
  my $hp=0;
  if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\$ax=uc(\$ax);$T";

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "uc: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }

  return ($code,$res);
}
