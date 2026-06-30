sub PD_loadtab($)
{ my $code=$_[0];
  my $res= ""; #"(EWload (";
  my $rr; ($code,$rr)=delperc(substr($code, 7));

#gestione parentesi 1/2
  my $hp=0;
  if (substr($code,0,1) eq "(")
  { ($code,$rr)=delperc(substr($code,1));
    $hp=1;
  }

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "loadtab: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }
  $res.= "\$tmp=\$ax; tab_wlock(\$tmp);$T";
  $res.= "\$ax=join(\"\\n\", tab_read(\$ax));$T";
  $res.= "tab_unlock(\$tmp);$T";
  return ($code,$res);
}
