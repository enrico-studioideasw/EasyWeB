sub PD_load($)
{ my $code=$_[0];
  my $res= ""; #"(EWload (";
  my $rr; ($code,$rr)=delperc(substr($code, 4));

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

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "load: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }

  $res.= "\$ax=EWload(\$ax);$T";
  return ($code,$res);
}
