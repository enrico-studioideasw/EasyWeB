sub PD_lessdate($)
{ my $code=$_[0];
  my $hp=0;
  my $res; #= "(EWlessdate(";
#se c'e' una parentesi la tolgo..
  my $rr; ($code,$rr)=delperc(substr($code, 8));
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
  { errore "lessdate: manca un parametro.";
  }
  ($code,$rr)=substr($code,1);
  #$res.= ",";
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "lessdate: manca ).";
    }
    my $rr; ($code,$rr)=delperc(substr($code,1));
  }
  $res.= "\$bx=pop \@as;$T\$ax=EWlessdate(\$bx,\$ax);$T";
  return ($code,$res);
}
