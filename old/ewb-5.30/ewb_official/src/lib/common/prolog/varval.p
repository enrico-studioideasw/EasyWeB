sub varval($$)
{ #legge il valore di una variabile.
  #se e' il valore di un altra variabile si richiama in ricorsivo.
  my ($a, $vars)=@_;
  my $l;
  my @vars=split("\n",$vars);
  my $res="<Null>";
  foreach $l(@vars)
  { my($name,$val)=split("#",$l);
    if ($name eq $a) {$res = $val;};
  }
  my $aa=substr($res,0,1);
  if ((($aa ge "A") && ($aa le "Z")) || ($a eq "_"))
  { $res=varval($res,$vars);
  }
  return $res;
}
