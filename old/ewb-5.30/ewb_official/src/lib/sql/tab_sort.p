my %sort;
sub tab_sort($$)
{ my ($name,$s)=@_;
  $name=((split(":", $name))[0]);
  #mi segno quale e' il campo su cui svolgere il sort di sql.
  #uso un hash.
  $sort{$name}=$s;
}
