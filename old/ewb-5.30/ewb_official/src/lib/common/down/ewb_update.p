sub ewb_update($$)
{ my ($n1, $k)=@_;
  my ($n)=split(":", $n1);
  my $res=tab_del($n, $k);
  if ($res ne "")
  { 
    return ewb_add($n1);
  }
  else { return ""; };
}
