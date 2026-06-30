sub whileblock($)
{ my $code=$_[0];
  $code=substr($code, 5);
  my $res="";

  my $nc=$num_calls;
  $num_calls=$num_calls+2;
  $res.= "\narea".$nc.":;$T";

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $rb; ($code,$rb)=codice($code); $res.=$rb;

  $res.= "if (! (\$ax";
  $res.= ")) {goto area".($nc+1).";};$T";

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code,0,1) ne "{") { errore "Necessaria \"{\" dopo while. "; }
  inblock();
  $code=substr($code, 1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  while ( substr($code, 0, 1) ne "}")
  { my $rb; ($code,$rb)=codice($code); $res.=$rb;
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
  $code=substr($code,1);
  outblock();
  $res.= "goto area$nc;$T";
  $res.= "\narea".($nc+1).":;$T";

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code,0,1) ne ";") { errore "Manca ; finale in while"; }
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  return ($code,$res);
}

sub downwhile($)
{ my $code=$_[0];
  $code=substr($code, 2);
  my $nc=$num_calls;
  $num_calls=$num_calls+1;
  my $res="";

  $res.= "area".$nc.":;$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code,0,1) ne "{") { errore "Necessaria \"{\" dopo do . "; }
  inblock();
  $code=substr($code, 1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  while ( substr($code, 0, 1) ne "}")
  { my $rb; ($code,$rb)=codice($code); $res.=$rb;
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
  $code=substr($code,1);

  outblock();
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code, 0,5) ne "while")
  { errore "necessario while dopo do {..}"; }
  $code=substr($code, 5);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $rb; ($code,$rb)=calcexp($code); $res.=$rb;
  $res.= "if (\$ax";
  $res.= ") {goto area".$nc.";};$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  #if (substr($code,1) ne ";") { errore "Manca ; finale in do..while"; }
  #my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  return ($code,$res);
}
