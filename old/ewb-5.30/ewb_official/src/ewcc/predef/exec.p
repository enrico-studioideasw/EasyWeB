sub PD_exec($)
{ my $code=$_[0];
  my $rr; ($code,$rr)=delperc(substr($code, 4));
  my $res;

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
    { errore "exec: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }


  $res.= "\$ax=qx{\$ax};$T";
  return ($code,$res);
}
