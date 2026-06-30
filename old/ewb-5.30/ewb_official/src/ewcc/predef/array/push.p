sub PD_push($)
{ my $code=$_[0];
  my $hp="";
  my $res="";
  my $rr; ($code,$rr)=delperc(substr($code, 4));
   if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }
 my $n=variable($code);
  ($code,$rr)=delperc(substr($code,length($n)));

  if (substr($code, 0, 1) ne ",")
  { errore "push: parametri stack, exp.sorgente.";
  }
  ($code,$rr)=delperc(substr($code, 1));

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.="\$EWB_$n=\$EWB_$n.toasc(\$ax).\" \";$T";
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "push: manca ).";
    }
    my $rr; ($code,$rr)=delperc(substr($code,1));
  }
  return ($code,$res);
}
