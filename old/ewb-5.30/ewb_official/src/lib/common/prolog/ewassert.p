sub EWassert($$)
{ my ($ruleset,$rule)=@_;
  my $rs=EWload(".RULE".$ruleset.".tab");
  my $res=assert($rule,$rs);
  if ($res) {EWsave(".RULE".$ruleset.".tab",$rs);};
  return $res;
}
