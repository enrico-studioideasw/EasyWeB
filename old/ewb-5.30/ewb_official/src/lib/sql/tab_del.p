sub tab_del($$)
{ my ($n,$k)=@_; my $r=""; my $l;
  tab_wlock($n);
  my $name=((split(":", $n))[0]);
  my $field=((split(":", $n))[1]);
  my $sql="DELETE * from $name where $field ='".delchar($k)."'";
  sqlrun(sql);
  tab_unlock($n); return $r;
}
