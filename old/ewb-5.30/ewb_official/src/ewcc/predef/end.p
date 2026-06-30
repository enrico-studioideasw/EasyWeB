sub PD_end($)
{ my $code=$_[0];
  #modifica 030424P inizio
  #my $res= "exit (\$ax);$T";
  my $res= "exit (0);$T";
  #modifica 030424P fine
  my $rr; ($code,$rr)=delperc(substr($code, 3));

#gestione parentesi 1/2
  my $hp=0;
  if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "end: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }

  return ($code,$res);
}
