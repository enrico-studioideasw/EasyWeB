sub PD_char($)
{ my $code=$_[0];
  my $res= "";  # "(chr (";
  my $rr; ($code,$rr)=delperc(substr($code, 4));

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
  $res.= "\$ax=chr(\$ax);$T";

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "char: manca ).";
    }
    ($code,$rr)=delperc(substr($code,1));
  }

  return ($code,$res);
}
