sub forblock($)
{ my $code=$_[0];
  my $res="";
#for (c1, c2, c3) {c}
#	c1
#   st:	if (c2) goto a
#       goto end
#    b: c3
#       goto st
#    a: c
#       goto b
#  end:
  my $nc=$num_calls;
  $num_calls=$num_calls+4;
  my $tmp;
  ($code,$tmp)=delperc(substr($code, 3));
  if (substr($code, 0,1) ne "(") { errore "necessaria ( dopo for."; }
  $code=substr($code, 1);
  my $rcd;($code,$rcd)=delperc($code);
  my $rb; ($code,$rb)=codice($code); $res.=$rb;
#tolgo la virgola.
  if (substr($code,0,1) ne ",")
  { errore "for: le condizioni vanno separate da virgola.";
  }
  $code=substr($code,1);

  $res.= "\narea".($nc+2).":;$T";

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  ($code,$rb)=codice($code); $res.=$rb;
#tolgo la virgola.
  if (substr($code,0,1) ne ",")
  { errore "for: le condizioni vanno separate da virgola.";
  }
  $code=substr($code,1);
  $res.= "if (\$ax";
  $res.= ") { goto area".$nc."; };$T";
  $res.= "goto area".($nc+3).";$T"; #end
  $res.= "\narea".($nc+1).":;$T";

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $rb; ($code,$rb)=codice($code); $res.=$rb;
  $res.= "goto area".($nc+2).";$T"; #st

  if (substr($code, 0,1) ne ")") { errore "manca )."; }
  $code=substr($code, 1);

  $res.= "\narea".($nc).":;$T";

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code,0,1) ne "{") { errore "Necessaria \"{\" dopo for. "; }
  inblock();
  $code=substr($code, 1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  while ( substr($code, 0, 1) ne "}")
  { my $rb;
    ($code,$rb)=codice($code);
    $res.=$rb;
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
  $code=substr($code,1);
  outblock();
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "goto area".($nc+1).";$T"; #st
  $res.= "area".($nc+3).":;$T";


  if (substr($code,0,1) ne ";") { errore "Manca ; finale in for"; }
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
  return ($code, $res);
}
