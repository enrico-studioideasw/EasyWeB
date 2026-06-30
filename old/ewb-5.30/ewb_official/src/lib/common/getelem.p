sub getelem($$)
{  my @a=split(/ /,$_[0]);
   $res=fromasc($a[$_[1]]);
   return $res;
}
