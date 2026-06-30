sub parsepart($)
{ #torna il funtore seguito dall' eventuale aperta parentesi,e da
  #un conteggio finche' non sichiude.
  my $l=$_[0];
  my $a;
  my $r = funtore $l;
  $l=substr($l,length($r));
  if (substr($l,0,1) eq "(")
  { #conteggio di parentesi.. avere una macchina di touring..
     my $cp=1; $l=substr($l,1);
     $r=$r."(";
     for($a=substr($l,0,1); $cp>0; $a=substr($l,0,1) )
     { $l=substr($l,1);
        if (($l eq "")&&($cp>1)) {die "$cp fine stringa inaspettato.";};
        if ($a eq "(") {$cp++};
        if ($a eq ")") {$cp--; };
        $r=$r.$a;
     }
  }
  return $r;
}
