sub PD_random($)
{ my $code=$_[0];
  my $res= "\$ax=(rand(1));$T";
#se ci sono parentesi me le magno.
  my $rr; ($code,$rr)=delperc(substr($code, 6));
  if (substr($code, 0,1) eq "(")
  { my $rr; ($code,$rr)=delperc(substr($code,1));
    if (substr($code, 0,1) ne ")")
    { errore "non servono parametri per random.";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
