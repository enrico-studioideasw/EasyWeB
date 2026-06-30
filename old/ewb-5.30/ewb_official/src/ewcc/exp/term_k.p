#gestione del suffisso di array a destra di un espressione.
sub term_k($)
{ my $code=$_[0];
  my $rb="";
if ($dbg) { printf "term_k ".substr($code, 0, 7)."\n";};
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
  ($code,$rcd)=term_c($code);$res.=$rcd;
  if (substr($code,0,1) eq "[")
  {
    $code=substr($code,1);
    ($code,$rcd)=delperc($code);$res.=$rcd;
    $res=$res."push \@as,\$bx;".$T."push \@as,\$ax;".$T;
    #qui codice
    my $rcd;($code,$rcd)=calcexp($code);$res.=$rcd;

    $res=$res."\$bx=pop \@as;$T\$ax=getelem(\$bx,\$ax);".$T."\$bx=pop \@as;$T";
    if (substr($code, 0, 1) ne "]")
    { errore "manca ']' in array.";
    };
    $code=substr($code,1);
    ($code,$rcd)=delperc($code);$res.=$rcd;
  } else
  { $res.=$rb;
  };
if ($dbg) { printf "term_k ret\n"; };
  return ($code, $res);
};
