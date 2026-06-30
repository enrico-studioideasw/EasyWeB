sub EWsplit($$)
{ my $a=""; my $p;
  foreach $p(split( $_[0], $_[1]))
  { $a=$a.toasc($p)." ";
  } return $a;
}

