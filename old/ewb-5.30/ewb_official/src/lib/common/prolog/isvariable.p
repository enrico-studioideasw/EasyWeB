sub isvariable($)
{ my $a=substr($_[0],0,1);
  return (($a ge 'A') && ($a le 'Z')) || ($a eq "_");
}

sub varassign($$$)
{ #ricorda l' associazione nome=valore.
  #valore puo' essere il nome di un altra variabile.Non della stessa.
  my ($a,$b, $vars)=@_;
  if ($a ne $b)
  { my $n=$a."#".$b;
    $vars=$vars.$n."\n";
    $_[2]=$vars;
  }
}
