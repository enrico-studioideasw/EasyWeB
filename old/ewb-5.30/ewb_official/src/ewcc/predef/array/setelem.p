sub PD_setelem($)
{ my $code=$_[0];
  my $hp;
  my $rr;
  ($code,$rr)=delperc(substr($code, 7));
   if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $n;
  ($code, $n)=variable($code);

  my $res="";
  if (substr($code, 0, 1) ne ",")
  { errore "setelem richiede tre parametri..";
  }
  ($code,$rr)=delperc(substr($code, 1));

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.="push \@as,\$ax;$T";
  if (substr($code, 0, 1) ne ",")
  { errore "setelem richiede tre parametri..";
  }
  ($code,$rr)=delperc(substr($code, 1));

  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.="\$bx=pop \@as;$T";
  $res.= "\$ax=setelem(\$EWB_$n,\$bx, \$ax);$T";
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "setelem: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
