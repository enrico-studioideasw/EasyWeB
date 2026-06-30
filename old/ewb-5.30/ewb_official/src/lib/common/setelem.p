sub setelem($$$)
{ my @a=split(/ /,$_[0]);
  $a[$_[1]] =toasc($_[2]);
  $_[0]=join(" ",@a);
}

