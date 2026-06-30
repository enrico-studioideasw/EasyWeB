sub PD_savetab($)
{ my $code=$_[0];
  my $res= ""; #"(EWsave (";
  my $rr; ($code,$rr)=delperc(substr($code, 7));

#gestione parentesi 1/2
  my $hp=0;
  if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb."push \@as,\$ax;$T";
  $res.= "\$tmp=\$ax;$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if (substr($code, 0, 1) ne ",")
  { errore "savetab: mi aspetto due parametri.";
  }
  ($code,$rr)=delperc(substr($code, 1));

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "tab_wlock(\$tmp);$T
          \$bx=pop \@as;$T\$ax=tab_save(\$bx,\$ax);$T
	  tab_unlock(\$tmp);$T";

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "savetab: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }

  return ($code,$res);
}
