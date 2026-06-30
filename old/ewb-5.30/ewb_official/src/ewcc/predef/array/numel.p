sub PD_numel($)
{ my $code=$_[0];
  my $rr;
  ($code,$rr)=delperc $code;
  my $res= ""; #"(\"\".(split (/ /,";
  $code=substr($code, 5);

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
  $res.= "\$ax=\"\".split(/ /,\$ax);$T";

#gestione parentesi 2/2
  if($hp)
  { if (substr($code,0,1) ne ")")
    { errore "numel: manca ).";
    }
    my $rr;
    ($code,$rr)=delperc(substr($code,1));
  }

  return ($code,$res);
}

