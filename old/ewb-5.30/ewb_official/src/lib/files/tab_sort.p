sub tab_sort($$)
{ my ($name,$s)=@_; my $r=""; my $l;
  my $n=((split(":",$name))[0]);
  $name=substr($name,length($n)+1);
  my $p; my $c=0;
  foreach $p(split(":",$name))
  { if ($p eq $s) { $k=$c; };
    $c++;
  }
  my @b; $c=0;
  tab_wlock($n);
  open III,"<".checkpath().".TB$n.tab";
  my @a=(<III>); close III;
  foreach $p(@a)
  { my $t;
    my @p=split("#",$p);
    $t=$p[0];
    $p[0]=$p[$k];
    $p[$k]=$t;
    $b[$c]=join("#",@p);
    $c=$c+1;
  }
  @b=sort(@b);
  $c=0;
  foreach $p(@b)
  { my $t;
    my @p=split("#",$p);
    $t=$p[0];
    $p[0]=$p[$k];
    $p[$k]=$t;
    $a[$c]=join("#",@p);
    $c=$c+1;
  }
  open oof,"> ".checkpath().".TB$n.tab";
  foreach $l(@a)
  { print oof $l;
  }
  close oof; tab_unlock($n); return $r;
}
