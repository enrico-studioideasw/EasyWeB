sub PD_sort($)
{ my $code=$_[0];
  my $hp=0;
  my $name;
  my $rr;
  ($code,$rr)=delperc(substr($code, 4));
  if (substr($code, 0,1) eq "(")
  { ($code,$rr)=delperc(substr($code, 1));
    $hp=1;
  }
  my $res= "";#(tab_sort(";

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if (substr($code, 0, 1) ne ",")
  { errore "sort richiede due parametri..";
  }
  ($code,$rr)=delperc(substr($code, 1));
  $res.= "push \@as,\$ax;$T";
  #il secondo parametro e' il nome del campo su cui fare il sort.

  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.= "\$bx=pop \@as;$T";
  $res.= "\$ax=tab_sort(\$bx,\$ax);";

  if (($hp==1) && (substr($code, 0, 1) ne ")"))
  { errore "sort - parentesi non chiusa.";
  }
  if ($hp) { ($code,$rr)=delperc(substr($code, 1)); }
  return ($code,$res);
}
