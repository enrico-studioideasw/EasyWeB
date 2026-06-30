#deve diventare
#term_f
#bx=ax
#term_f
#ax=bx operator ax
sub term_t($ )
{ my $code=$_[0];
  my $res="";
if ($dbg) { printf "term_t ".substr($code, 0, 7)."\n"; };
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  ($code,$res)=term_f($code);
  if ((substr($code, 0,1) eq "*")
      || (substr($code, 0,1) eq "/")
      || (substr($code, 0,1) eq "%"))
  { my $op= substr($code, 0, 1);
    $code=substr($code, 1);
    $res.="push \@as,\$ax;$T";
    my $rb; ($code,$rb)=term_t($code);
    $res.=$rb."\$bx=pop \@as;$T\$ax=\$bx $op \$ax;$T";
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
if ($dbg){  printf "term_t ret\n"; }; 
  return ($code,$res);
}
