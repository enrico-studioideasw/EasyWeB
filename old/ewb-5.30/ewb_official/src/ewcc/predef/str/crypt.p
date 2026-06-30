sub PD_crypt($)
{ my $code=$_[0];
  my $res="";
  my $hp=0;
#  $res.= "(crypt(";
#se c'e' una parentesi la tolgo..
  my $rr; ($code,$rr)=delperc(substr($code, 5));
  if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  $res=$res.$rb."push \@as,\$ax;$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

#  print "[$code]\n"; exit(0);
  if (substr($code, 0, 1) ne ',')
  { errore "crypt: manca un parametro.";
  }
  $code=substr($code,1);
    my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  $res.= "\$bx=pop \@as;$T\$ax=crypt(\$bx,\$ax);$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "crypt: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
