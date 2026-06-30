sub PD_swrite($)
{ my $code=$_[0];
  my $hp="";
  my $rr; ($code,$rr)=delperc(substr($code, 6));
   if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $res= "";#"(print ";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "push \@as,\$ax;$T";
  if (substr($code, 0, 1) ne ",")
  { errore "swrite richiede due parametri..";
  }
  ($code,$rr)=delperc(substr($code, 1));

  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.= "\$bx=pop \@as;$T";
  #$res.= "print \$bx \$ax;$T";
  $res.= "\$ax=print \$bx \$ax;$T";
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "swrite: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
