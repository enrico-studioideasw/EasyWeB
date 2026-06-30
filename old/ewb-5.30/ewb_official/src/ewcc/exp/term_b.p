sub term_b($)
{ my $code=$_[0];
  my $res="";
if ($dbg) { printf "term_b ".substr($code, 0, 7)."\n"; };

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
  ($code,$res)=term_k($code);
  if (substr($code, 0,2) eq "&&")
  { my $op= substr($code, 0, 2);
    $res.="push \@as,\$ax;$T";
    $code=substr($code, 2);
    my $rb; ($code,$rb)=term_b($code);
    $res.=$rb."\$bx=pop \@as;$T\$ax=\$bx $op \$ax;$T";
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
  }
  elsif (substr($code, 0,1) eq "&")
  { my $op= substr($code, 0, 1);
    $code=substr($code, 1);
    my $rb;($code,$rb)=term_b($code);
    $res.=$rb."\$bx=pop \@as;$T\$ax=\$bx $op \$ax;$T";
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
if ($dbg) {printf "term_b ret\n"; };

  return ($code,$res);
}
