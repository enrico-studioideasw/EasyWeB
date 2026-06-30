sub PD_server($)
{ my $code=$_[0];
  my $hp="";
  my $rr; ($code,$rr)=delperc(substr($code, 6));
   if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $res= "";#"(EWserver(";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\$ax=EWserver(\$ax);$T";
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "Server socket: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
