#funzione ad  uso interno.
sub geturl($)
{ #leggo stupidamente tutti i caratteri fino al primo che non puo' farne parte.
  #url: [a..z], [A..Z], [0..9], -, :, / , .
  my ($code,$rr)=delperc($_[0]);
  my $c; my $url="";
  $c=substr($code, 0, 1);
  while (
    (($c ge "a") && ($c le "z")) ||
    (($c ge "A") && ($c le "Z")) ||
    (($c ge "0") && ($c le "9")) ||
    ($c eq "-") ||
    ($c eq "_") ||
    ($c eq "%") ||
    ($c eq ":") ||
    ($c eq "/") ||
    ($c eq "."))
  { $url=$url.$c;
#print $url."\n";
    $code=substr($code, 1);
    $c=substr($code,0,1);
  }
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  return ($code,$url);
}
