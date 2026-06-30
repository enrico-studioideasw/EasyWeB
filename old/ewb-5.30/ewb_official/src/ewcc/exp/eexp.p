sub eexp($)
{ my $code=$_[0];
  my $res;
if ($dbg) {printf "eexp ".substr($code, 0, 7)."\n";};
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  ($code,$res)=term_e($code);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code, 0,1) eq ".")
  { #print ".";
    my $rb;
    $res.="push \@as,\$ax;$T";
    $code=substr($code, 1);
    ($code,$rb)=eexp($code);
    $res.=$rb;
    $res.="\$bx=pop \@as;$T\$ax=\$bx.\$ax;$T";
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  }
if ($dbg) {  printf "eexp ret\n"; }; 

  return ($code,$res);
}
