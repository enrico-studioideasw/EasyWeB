sub PD_lock($)
{ my $code=$_[0];
  my $hp=0;
  my $name;

  my $rr; ($code,$rr)=delperc(substr($code, 4));
  if (substr($code, 0,1) eq "(")
  { ($code,$rr)=delperc(substr($code, 1));
    $hp=1;
  }
  my $res= "";#(tab_wlock(";
  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\$ax=tab_wlock(\$ax);$T";

  if (($hp==1) && (substr($code, 0, 1) ne ")"))
  { errore "lock - parentesi non chiusa.";
  }
#codice di lock.. dalla libtable.
  if ($hp) { my $rr; ($code,$rr)=delperc(substr($code, 1)); }
  return ($code,$res);
}
