sub tab_del($$)
{ my ($n,$k)=@_; my $r=""; my $l;
  tab_wlock($n);
  open III,"< ".checkpath().".TB$n.tab";
  my @a=(<III>); close III;
  open oof,"> ".checkpath().".TB$n.tab";
  foreach $l(@a)
  {
    if (substr($l,length($l)-1,1) eq "\n")
    {  $l=substr($l, 0, length($l)-1);
    }
    if ( ((split("#",$l))[0]) eq delchar($k))
    { $r=1;}
    else
    { print oof $l."\n";
    }
  }
  close oof; tab_unlock($n); return $r;
}
