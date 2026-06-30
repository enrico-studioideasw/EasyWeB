sub ifblock($)
{ my $code=$_[0];
  my $res="";
  $code=substr($code,3);

#if (a) {}
#elseif b {}
#else {c}
#	if (!a) goto uno  {}
#	goto tre
#	uno:
#	if (!b) goto due  {}
#	goto tre
#	due:
#	{c}
#	tre:

  #calcexp
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  ($code,$rcd)=calcexp($code); $res.=$rcd;
  $res.= "if (!(\$ax";
  $res.= ")) { goto area$num_calls; };$T";
  my $dest1=$num_calls;
  ++$num_calls;
  #mi aspetto { ,[codice] , }
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code,0,1) ne "{") { errore "Necessaria \"{\" dopo if. "; }
  inblock();

  $code=substr($code, 1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  while ( substr($code, 0, 1) ne "}")
  { my $rb;
    ($code, $rb)=codice($code);
    $res.=$rb;

    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  }
  $code=substr($code,1);
  outblock();
  my $nc=$dest1;
  $res.= "goto areaend$nc;$T";


  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  $res.= "\narea$dest1:;$T";

  #se manca il punto e virgola deve esserci un else o elseif
  if (substr($code,0,1) eq ";") { $code=substr($code,1); $res.= ";\n"; }

  else
  {
   while (substr($code, 0,6) eq "elseif")
   { $code=substr($code, 6);
     my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
     my $rb;
     ($code,$rb)=calcexp($code);
     $res.=$rb;
     $res.= "if (!(\$ax";
     $res.= ")) { goto area$num_calls; };$T";
     my $dest=$num_calls;
     ++$num_calls;
     my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
     if (substr($code,0,1) ne "{") { errore "Necessaria \"{\" dopo elseif. "; }
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

     $res.= "goto areaend$nc;$T";
     $res.= "\narea$dest:;$T";

     my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
   }
   if (substr($code, 0,4) eq "else")
   { $code=substr($code, 4);
     my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
     my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
     if (substr($code,0,1) ne "{") { errore "Necessaria \"{\" dopo elseif. "; }
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
   }
   if (substr($code,0,1) ne ";") { errore "Manca ; finale in if.. else (b)"; }

  }
  $res.= "\nareaend$nc:;$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  return ($code,$res);
}
