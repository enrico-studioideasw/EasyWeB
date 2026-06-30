sub PD_checktable($)
{ my $t;
  my $res=0;
  foreach $t(@tables)
  { my $a=((split(":", $t))[0]);
    if ($a eq $_[0]) { $res=1; };
  }
  if (!$res) { errore "Tabella inesistente."; };
}
