sub EWretract($$)
{ #elimina le regole con la testa indicata.
  #se non e' indicata la testa le elimina tutte.
  my ($ruleset,$rule)=@_;
  my $rs=EWload(".RULE".$ruleset.".tab");
  my $rsb; my $r;
  foreach $r(split("\n",$rs))
  { my $k="";
    if (($rule ne "")&&(!match($rule, $r,$k))) { $rsb=$rsb.$r."\n";};
  }
  EWsave(".RULE".$ruleset.".tab", $rsb);
}
