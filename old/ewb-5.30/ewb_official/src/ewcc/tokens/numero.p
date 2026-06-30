sub is_numvalue($)
{ my $code=$_[0];
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $a=substr($code, 0, 1);
  return ( ((uc($a) ge "0") && ( uc($a) le "9")) || ($a eq "-"));
}
#*** va bene cosi'
sub numvalue($)
{ my $code=$_[0];
  my $res=""; my $r;
  # [-] {0..9} [.{0..9} [0..9]]
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  #puo' iniziare con un segno, 0x, 0b
  if (substr($code, 0, 1) eq "-")
  { $res=$res. "-"; my $rest; ($code,$rest)=delperc(substr($code,1));
  }
  elsif (substr($code, 0, 2) eq "0x")
  { $res=$res. "0x"; my $rest; ($code,$rest)=delperc(substr($code,2));
  }
  elsif (substr($code, 0, 2) eq "0b")
  { $res=$res. "0b"; my $rest; ($code,$rest)=delperc(substr($code,2));
  }

  my $a=substr($code, 0, 1);
  if ((uc($a) lt "0") || ( uc($a) gt "9"))
  { errore "mi aspettavo un numero.";
  }
  my $r=1;
  do
  {
    my $a=substr($code, 0, 1);
    if ((uc($a) ge "0") && ( uc($a) le "9"))
    { $res=$res. $a;
      $code=substr($code,1);
    }
    else {$r=0;}
  }
  while ($r!=0);
  $a=substr($code,0,1);

  #se trovo un punto c'e' la parte frazionaria.
  if ($a eq ".")
  { $res.=".";
    $code=substr($code,1);
    $a=substr($code,0,1);

    if ((uc($a) lt "0") || ( uc($a) gt "9"))
    { errore "mi aspettavo un numero.";
    }
    do
    {
      $a=substr($code, 0, 1);
      if ((uc($a) ge "0") && ( uc($a) le "9"))
      { $res=$res. $a;
        $code=substr($code,1);
      }
    }
    while (($a ge "0") & ( $a le "9"));
  }
  return ($code,$res);
}
