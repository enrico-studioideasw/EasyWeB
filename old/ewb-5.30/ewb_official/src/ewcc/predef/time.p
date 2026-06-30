sub PD_time($)
{ my $code=$_[0];
  my $res= "\$ax=(EWtime ());$T";
#se ci sono parentesi me le magno.
  my $rr; ($code,$rr)=delperc(substr($code, 4));
  if (substr($code, 0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    if (substr($code, 0,1) ne ")")
    { errore "non servono parametri per time.";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
