sub ewb_remove($)
{ my @t=split(":", $_[0]);
  $res="";
  my $code="\$res=tab_del(\"".$t[0]."\",\$EWB_".$t[1].");";
  eval($code);
  return $res;
};
