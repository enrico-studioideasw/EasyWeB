sub PD_socket($)
{ my $code=$_[0];
  my $hp="";
  my $rr; ($code,$rr)=delperc(substr($code, 6));
   if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $res= "";#(EWsocket(";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "push \@as,\$ax;";
  if (substr($code, 0, 1) ne ",")
  { errore "socket richiede due parametri..";
  }
  ($code,$rr)=delperc(substr($code, 1));

  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\$bx=pop \@as;$T";
  $res.= "\$ax=EWsocket(\$bx,\$ax);$T";
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "socket: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
