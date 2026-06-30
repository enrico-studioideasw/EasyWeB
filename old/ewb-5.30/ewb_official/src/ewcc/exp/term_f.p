#deve diventare
#term_b
#bx=ax
#term_b
#ax=bx operator ax
sub term_f($)
{ my $code=$_[0];
  my $res;
if ($dbg) { printf "term_f ".substr($code, 0, 7)."\n"; };

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  ($code,$res)=term_b($code);
  if (substr($code, 0,2) eq "||")
  { my $op= substr($code, 0, 2);
    $code=substr($code, 2);
    $res.="push \@as,\$ax;$T";
    my $rb; ($code,$rb)=term_f($code);
    $res.=$rb."\$bx=pop \@as;$T\$ax=\$bx $op \$ax;$T";
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
  elsif (substr($code, 0,1) eq "|")
  { my $op= substr($code, 0, 1);
    $code=substr($code, 1);
    $res.="push \@as,\$ax;$T";
    my $rb; ($code,$rb)=term_f($code);
    $res.=$rb."\$bx=pop \@as;$T\$ax=\$bx $op \$ax;$T";
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
if ($dbg) { printf "term_f ret\n"; }; 

  return ($code,$res);
}
