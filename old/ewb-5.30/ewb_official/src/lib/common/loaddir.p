sub EWloaddir($)
{ my $a=""; my $el;
  my @f;
  if (opendir(dirr, $_[0]))
  { @f=readdir(dirr);
    foreach $el(@f)
    { $a=$a.toasc($el)." "; };
  }
  return $a;
}
