sub PD_pop($)
{ my $code=$_[0];
  my $hp="";
  my $rr; ($code,$rr)=delperc(substr($code, 3));
   if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }

  my $n;
  ($code, $n)=variable($code);
  my $res= "\$ax=stpop(\$EWB_$n);$T";
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "pop: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
