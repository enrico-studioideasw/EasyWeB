sub inexp($)
{ my $code=$_[0];
  my $res="";
if ($dbg) { printf "inexp ".substr($code, 0, 7)."\n"; };
  my $rb;
  my $rcd;($code,$rcd)=delperc($code);
  my $k=$res;
  ($code,$rb)=eexp($code);
  $res.=$rb;
#print "#db exp($code)\n";
  my $rcd;($code,$rcd)=delperc($code);
  while (is_cfr($code))
  { my $cf; my $rb;
    my $nmv=substr($res, 4, length($res));
    my $nmv=substr($nmv, 1, length($nmv)-4);
    $res.="push \@as,\$ax;$T";
    ($code,$cf)=cfr($code);
      #caso di confronto
      my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
      ($code,$rb)=eexp($code);
      $res.=$rb;
      $res.="\$bx=pop \@as;$T";
      $res.="\$ax=(\$bx $cf \$ax);$T";
  }
if ($dbg) {printf "inexp ret\n"; }; 

  return ($code,$res);
}
