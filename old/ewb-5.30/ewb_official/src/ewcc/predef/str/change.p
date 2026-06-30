sub PD_change($)
{ my $code=$_[0];
  my $hp=
#  my $res= "(join (";
#se c'e' una parentesi la tolgo..
  my $rr; ($code,$rr)=delperc(substr($code, 6));
  if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb."push \@as,\$ax;$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if (substr($code, 0, 1) ne ',')
  { errore "change - manca un parametro.";
  }
  $code=substr($code,1);
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb."push \@as,\$ax;$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

#print "[$code]\n";exit(0);
  if (substr($code, 0, 1) ne ',')
  { errore "change - manca un parametro.";
  }
  my $rr; ($code,$rr)=delperc(substr($code,1));
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  #modifica 030422P inizio
  #$res.= "\$bx=pop \@as;$T\$cx=pop \@as;$T \$as=join(\$cx,split(\$bx,\$ax));$T";
  $res.= "\$bx=pop \@as;$T\$cx=pop \@as;$T 
  \$ax=join(\$cx, split(\$bx,\$ax));$T";
  #modifica 030422P fine

  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "change: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
