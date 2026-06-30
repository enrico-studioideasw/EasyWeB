sub PD_mid($)
{ my $code=$_[0];
  my $hp=0;
#tre parametri. Uso ax, bx, cx
#  my $res= "(substr (";
#se c'e' una parentesi la tolgo..
  my $rr; ($code,$rr)=delperc(substr($code, 3));
  if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
    my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb."push \@as,\$ax;$T";

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

#  print "[$code]\n"; exit(0);
  if (substr($code, 0, 1) ne ',')
  { errore "manca un parametro.";
  }
  $code=substr($code,1);

    my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb."push \@as,\$ax;$T";

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if (substr($code, 0, 1) eq ',')
  {
    $code=substr($code,1);
    my $rb;
    ($code,$rb)=(calcexp($code));
    $res=$res.$rb;
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
    $res.= "\$bx=pop \@as;$T\$cx=pop\@as;$T\$ax=substr(\$cx,\$bx,\$ax);$T";
  }
  else
  { $res.= "\$bx=pop \@as;$T\$cx=pop\@as;$T\$ax=substr(\$cx,\$bx);$T";
  };
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "mid: manca ).";
    }
    my $rr; ($code,$rr)=delperc(substr($code,1));
  }

  return ($code,$res);
}
