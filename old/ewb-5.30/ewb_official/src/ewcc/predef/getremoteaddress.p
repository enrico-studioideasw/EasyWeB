#####
sub PD_GetRemoteAddress($)
{ my $code=$_[0];
  my $res= "\$ax=(EWgetenvvar(\"REMOTE_ADDR\"));$T";
  my $o; 
  ($code,$o)=delperc(substr($code, 16));
  
#gestione parentesi 1/2
  my $hp=0;
  if (substr($code,0,1) eq "(")
  { ($code,$o)=delperc(substr($code,1));
    $hp=1;   
  } 

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "remoteaddress: manca ).";
    }
    ($code,$o)=delperc(substr($code,1));
  }
  
  return ($code,$res);
};
