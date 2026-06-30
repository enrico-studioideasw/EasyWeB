sub EWjoin($$)
{ my @a;
  foreach $p(split(/ /,$_[1]))
  { @a[$#a+1]=fromasc($p); }
  return join($_[0],@a);
}
