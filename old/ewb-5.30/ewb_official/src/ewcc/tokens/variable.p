#___________________________________________________________________
#token elementari
#questo e' ad uso praticamente sempre interno.
sub variable($)
{ #torna il nome della variabile. Adattato al nuovo modo.
  my $code=$_[0];
  my $name="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $a=substr($code, 0, 1);
  if ( ((uc($a) ge "A") && ( uc($a) le "Z")) || ($a eq "_"))
  { $name=$name.$a;
    $code=substr($code, 1);
    $a=substr($code, 0, 1);
  }
  else
  { errore "Nome di variabile errato.";
  }

  while ( ((uc($a) ge "A") && ( uc($a) le "Z")) || ($a eq "_") || (($a ge "0")&&($a le "9")))
  { $name=$name.$a;
    $code=substr($code, 1);
    $a=substr($code, 0, 1);
  }
  if (length($name) eq "") { errore "funtore non trovato."; }
  return ($code, $name);
}

sub is_identifier($)
{
  my $code=$_[0];
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $a=substr($code, 0, 1);
  return ( ((uc($a) ge "A") && ( uc($a) le "Z")) || ($a eq "_"));
}

