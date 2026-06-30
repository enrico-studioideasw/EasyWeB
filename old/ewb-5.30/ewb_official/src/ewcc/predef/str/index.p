sub PD_index($)
{ my $code=$_[0];
  my $hp=0;
# my $res= "\$ax(index(
#se c'e' una parentesi la tolgo.
  my $rr; ($code,$rr)=delperc(substr($code, 5));
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
  #$res.= ",";

  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "index: manca ).";
    }
    #my $res;  ($code,$rr)=delperc(substr($code,1));
    my $rr;  ($code,$rr)=delperc(substr($code,1));
  }
  $res.= "\$bx=pop \@as;$T\$ax=index(\$bx,\$ax);$T";
  return ($code,$res);
}
