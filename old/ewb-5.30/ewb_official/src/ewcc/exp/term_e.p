#deve diventare
#ax=term_t <- lo fa terme
#bx=ax
#ax=term_t <-...
#ax=bx operator ax

sub term_e($ )
{ my $code=$_[0];
  my $res="";
if ($dbg) {printf "term_e ".substr($code, 0, 7)."\n"; };

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  ($code,$res)=term_t($code);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if ((substr($code, 0,1) eq "+") || (substr($code, 0,1) eq "-"))
  { my $op= substr($code, 0, 1);
    my $rb;
    $res.="push \@as,\$ax;$T";
    $code=substr($code, 1);
    ($code,$rb)=term_e($code);
    $res.="$rb\$bx=pop \@as;$T\$ax=\$bx $op \$ax;$T";

    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
if ($dbg)  { printf "term_e ret\n"; };

  return ($code,$res);
}
