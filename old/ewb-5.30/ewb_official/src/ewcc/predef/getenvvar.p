###############
#funzioni ereditate dalla 2.10 Inizio
sub PD_GetEnvVar($)
{ my $code=$_[0];
  my $rcd;
  my $res;  
  ($code, $rcd)=delperc(substr($code, 9));
  $res.=$rcd; 
#gestione parentesi 1/2
  my $hp=0;
  if (substr($code,0,1) eq "(")
  { ($code,$rcd)=delperc(substr($code,1));
    $res.=$rcd;
    $hp=1;
  }
  ($code,$rcd)=calcexp($code);
  $res.=$rcd; 
  ($code,$rcd)=delperc($code);
  $res.=$rcd; 

  $res.= "\$ax=EWgetenvvar(\$ax);$T";

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "envvar: manca ).";
    }
    ($code, $rcd)=delperc(substr($code,1));
    $res.=$rcd;
  }
  return ($code, $res);
};
