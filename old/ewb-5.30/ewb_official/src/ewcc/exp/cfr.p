sub is_cfr($)
{ my $code=$_[0];
  my $a=substr($code, 0, 1);
  my $b=substr($code, 0, 2);
  return ( ($a eq "<") || ($a eq ">") ||
           ($b eq "==")|| ($b eq "<=")|| ($b eq ">=")||
           ($b eq "ge")|| ($b eq "le")|| ($b eq "lt")||
           ($b eq "gt")||
           ($b eq "eq")|| ($b eq "ne")|| ($b eq "!="));
}
#***ok
sub cfr($)
{ my $code=$_[0];
  my $res;

  my $a=substr($code, 0, 1);
  my $b=substr($code, 0, 2);
  if      (($b eq "==")|| ($b eq "<=")|| ($b eq ">=")||
           ($b eq "ge")|| ($b eq "le")|| ($b eq "lt")||
           ($b eq "gt")||
           ($b eq "eq")|| ($b eq "ne")|| ($b eq "!="))
  { $res= $b; $code=substr($code, 2); }
  elsif  (($a eq "=") || ($a eq "<") || ($a eq ">"))
  { $res= "".$a.""; $code=substr($code, 1);
  }
  return ($code,$res);
}
