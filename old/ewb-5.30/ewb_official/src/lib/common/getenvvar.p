sub EWgetenvvar($)
{
  my $r;
  $r= $ENV{$_[0]};
  return $r;
}
