sub tab_read($)
{ #esegue la lettura di una tabella gia bloccata.
  my @t=split(":", $_[0]);
  my $sql="SELECT * from $t[0]";
  my @r=sqlrun($sql);
  return @r;
};
