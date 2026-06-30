sub PD_sread($)
{ my $code=$_[0];
  my $hp="";
  my $rr; ($code,$rr)=delperc(substr($code, 5));
   if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $res= "";#(EWsread(";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\$ax=EWsread(\$ax);$T";
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "sread: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
