sub EWsave($$)
{ my ($a, $b)=@_;
  my $r;
  $r = open oof, "> $a";
  if (!$r) { print "File di save non scrivibile. Controllare i permessi"; exit(0); };
  print oof $b;
  close oof;
  return $r;
}
#2.10 Inizio
