sub tab_read($)
{ #esegue la lettura di una tabella gia bloccata.
  my @t=split(":", $_[0]);
  open III,"< ".checkpath()."TB$t[0].txt";
    my $k=join("",<III>);
    my @r=split("\n",$k);
  close III;
  return @r;
};
