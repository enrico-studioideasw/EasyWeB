sub term_c($)
{ my $code=$_[0];
  my $found=0;
if ($dbg) { printf "term_c ".substr($code, 0, 7)."\n"; };

  my $rcd;($code,$rcd)=delperc($code);
  my $res;

  if (substr($code, 0, 1) eq "!")
  { # "!" - not unario
    #in questo caso diventa:
    #calcexp
    #ax=!ax;

    my $rest;
    my $rb;
    ($code,$rest)=delperc(substr($code,1));
    ($code,$rb)=inexp($code);
    $res.="$rb\$ax=!\$ax;$T";
    $found=1;
  }
  elsif ((!$found)&&(substr($code, 0, 1) eq "("))
  { #parentesi
    #in questo caso mi limito ad eseguire calcexp.

    #print "#(\n";
    $code=substr($code, 1);
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
    ($code,$res)=calcexp($code);
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
    if (substr($code,0, 1) ne ")")
    { errore "manca parentesi chiusa";
    }
    $code=substr($code, 1);
    $found=1;
  }
  elsif ((!$found)&&(is_tabella($code)))
  { ($code,$res)=tabella($code);
    $found=1;
  }
  elsif ((!$found)&&(is_identifier($code)))
  {
    ($code,$res)=identifier($code);
    $found=1;
  }
  elsif ((!$found)&&(is_numvalue($code)))
  { my $rb;
    ($code,$rb)=numvalue($code);
    $res="\$ax=$rb;$T";
    $found=1;
  }
  elsif ((!$found)&&(is_strvalue($code)))
  { my $rb;
    ($code,$rb)=strvalue($code);
    $res="\$ax=$rb;$T";
    $found=1;
  }
  elsif (!$found)
  { my $t=substr($code, 0, 4);
    errore "termine sconosciuto [$t ...]. ";
  }
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
if ($dbg) { printf "term_c ret\n"; }; 
return ($code,$res);
}
