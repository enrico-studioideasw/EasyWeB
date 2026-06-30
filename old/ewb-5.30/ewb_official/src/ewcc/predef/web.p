sub webask($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $a=variable($code);
  if ($a eq "ask")
  { ($code,$res)=PD_ask($code);
  }
  elsif ($a eq "show")
  { ($code,$res)=PD_show($code);
  }
  elsif ($a eq "askto")
  { ($code,$res)=PD_askto($code);
  }
  elsif ($a eq "form")
  { ($code,$res)=PD_form($code);
  }
  elsif ($a eq "showform")
  { ($code,$res)=PD_showform($code);
  }

##qui collegamento al motore inferenziale.
  else { ($code,$res)=prolog($code); };
  my $t; ($code,$t)=delperc($code);
  return ($code, $res);
}
