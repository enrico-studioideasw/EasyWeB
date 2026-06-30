#funzioni del motore inferenziale stile prolog.
sub prolog($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my ($tmp,$a)=variable($code);
  if ($a eq "assert")
  { ($code,$res)=PD_assert($code);
  }
  elsif ($a eq "retract")
  { ($code,$res)=PD_retreat($code);
  }
  elsif ($a eq "goal")
  { ($code,$res)=PD_goal($code);
  }
  my $t; ($code,$t)=delperc($code);
  return ($code, $res);
}
#queste sono interne.
