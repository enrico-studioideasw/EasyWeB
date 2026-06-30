sub PD_join($)
{ my $code=$_[0];
  my $hp=0;
  #my $res= "(EWjoin (";
#se c'e' una parentesi la tolgo..
  my $rr; ($code,$rr)=delperc(substr($code, 4));
  if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb."push \@as,\$ax;$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if (substr($code, 0, 1) ne ',')
  { errore "manca un parametro.";
  }
  $code=substr($code,1);
  ($code,$rb)=(calcexp($code));
  ($code,$rr)=delperc($code);
  $res.=$rb;
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "index: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  $res.= "\$bx=pop \@as;$T\$ax=EWjoin(\$bx,\$ax);$T";
  return ($code,$res);
}
