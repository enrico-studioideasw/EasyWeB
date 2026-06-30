sub calcexp($)
{ my $code=$_[0];
  if ($dbg) { printf "calcexp ".substr($code, 0, 7)."\n"; };
  my $res="";
  my $rb;
  my $rcd;($code,$rcd)=delperc($code);
  if (is_assign($code))
  {   my $rcd;($code,$rcd)=assign($code);$res.=$rcd;
  }
  else
  { ($code,$rcd)=inexp($code);$res.=$rcd;
  };
if ($dbg) { printf "calcexp ret\n"; };

  return ($code, $res);
};
