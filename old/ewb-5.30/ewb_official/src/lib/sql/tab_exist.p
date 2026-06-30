sub tab_exist($$)
{ my ($n,$k)=@_;
  tab_wlock($n);
  my @a=split(":", $n);
  my $sql="SELECT * from @a[0] where @a[1]='".delchar($k)."'";
  my @risp=sqlrun($sql);
  if ($#risp>=0)
  { @r=split("#",$risp[$#risp]);
    foreach $l(@r)
    { $l=addchar($l);
    };
  }
  else { $r = ""; };
  tab_unlock($n); return @r;
}
