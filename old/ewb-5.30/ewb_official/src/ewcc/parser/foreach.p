sub foreachblock($)
{ #complicato: devo generare una variabile interna e una temporanea di loop.
  # foreach $int($a) {work;};
  # my @a_foreachcount=split(" ", $a)
  # uno:
  # if (@a_foreachcount eq "" ) goto due;
  # $int=fromasc(@a_foreachcount);
  # @a_foreachcount=@foreachcount - primo elemento;
  # goto uno;
  # due:
  my $code=$_[0];
  my $res="";
  my $nc=$num_calls;
  $num_calls=$num_calls+3; #2 calls, 1 temporanea
  $code=substr($code, 7);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  inblock();

  $res=$res. $T."my \$";
  my $name;
  ($code, $name)=variable($code);
  checkpredef ($name);
  $res=$res. "EWB_".$name;
  $res=$res. ";$T";
  #aggiungo la variabile in ambiente.
  $variables[$#variables+1]= $name.":v";

  #aggiungo la temporanea per forblock
  $res=$res."my \$EWB_forblockvar_".$nc.";$T";
  $variables[$#variables+1]= "forblockvar".$nc.":v";
  #valuto il codice del blocco di for.
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  if (substr($code, 0, 1) ne "(")
  { errore "foreach: e' prevista aperta parentesi dopo la variabile.";
  };
  $code=substr($code,1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $rb; ($code, $rb)=calcexp($code); $res.=$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code, 0, 1) ne ")")
  { errore "foreach: blocco di codice non corretto.";
  };
  $res=$res."my \$EWB_forblockvar".$nc." = \$ax.\" \";$T";
  $code=substr($code, 1);
  my $rb; ($code, $rb)=delperc($code); $res.=$rb;
  $res.= "area".($nc+1).":;$T";

  $res.="if (\$EWB_forblockvar".$nc." eq \"\") { goto area".($nc+2).";};$T";

  $res.="\$EWB_".$name." = getelem(\$EWB_forblockvar".$nc.",0);$T";
  $res.="\$EWB_forblockvar".$nc."=substr(\$EWB_forblockvar".$nc.", index(\$EWB_forblockvar".$nc.", \" \")+1);$T";


#qui codice
  if (substr($code,0,1) ne "{") { errore "Necessaria \"{\" dopo foreach. "; }
  $code=substr($code, 1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  while ( substr($code, 0, 1) ne "}")
  { my $rb; ($code,$rb)=codice($code); $res.=$rb;
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  };

  $code=substr($code,1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code,0,1) ne ";") { errore "Manca ; finale in while"; }
  $code=substr($code,1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.="goto area".($nc+1).";$T";
  $res.= "area".($nc+2).":;$T";

  outblock();
  return ($code,$res);

};
