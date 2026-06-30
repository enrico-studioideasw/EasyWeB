sub funtore($)
{ #torna la parte della stringa composta di caratteri a..z, A..Z, 1..9, _
  my $l=$_[0];
  my $a;
  my $r;
  for ($a=substr($l,0,1);
        ( ((uc($a) ge "A") && (uc($a) le "Z")) || (($a ge"0") &&  ($a le "9")) || ($a eq "_") || 
($a eq "~"));
        $a=substr($l,0, 1) )
        { $r=$r.$a;
           $l=substr($l,1);
        };
  return $r;
}
