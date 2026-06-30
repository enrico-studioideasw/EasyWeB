sub PD_unlock($)
{ my $code=$_[0];
  my $hp=0;
  my $name;
  my $rr;
  ($code,$rr)=delperc(substr($code, 6));
  if (substr($code, 0,1) eq "(")
  { ($code,$rr)=delperc(substr($code, 1));
    $hp=1;
  }

  my $res= "";#"(tab_unlock(";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\$ax=tab_unlock(\$ax);$T";

  if (($hp) && (substr($code, 0, 1) ne ")"))
  { errore "unlock - parentesi non chiusa.";
  }
  if ($hp) { ($code,$rr)=delperc(substr($code, 1)); }
  return ($code, $res);
}
