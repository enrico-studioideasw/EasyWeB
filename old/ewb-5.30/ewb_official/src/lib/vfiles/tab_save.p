sub tab_save($$)
{ #esegue la scrittura di una tabella gia bloccata.
  my @t=split(":", $_[0]);
  open III,"> ".checkpath()."TB$t[0].txt";
     print III $_[1]."\n";
  close III;
};
