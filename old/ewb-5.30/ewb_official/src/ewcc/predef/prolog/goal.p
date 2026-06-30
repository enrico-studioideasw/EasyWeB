#Clausola di goal
sub PD_goal($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $a; ($code,$a)=variable($code);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $hp;
  $res.= "my \$rs=EWload(\".RULE";
  if (substr($code, 0,1) eq "(")
  { ($code,$rcd)=delperc(substr($code, 1));
    $hp=1;
  }
#qui una variabile anziche' il codice.
  ($code,$a)=variable($code);
  checkpredef($a);
  $res.= $a.".tab\");
  \$rlist=\"\"; my \$vars=\"\";$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  #Qui ho bisogno di pescare il codice che genera la clausola.
  if ((substr($code, 0, 1) ne ","))
  { errore "goal - manca un parametro.";
  }
  ($code,$rcd)=delperc(substr($code,1));

  my $rb;
  ($code,$rb)=(calcexp($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.="solve(\$ax,";
  $res.= "\$rs,\$vars);$T"."my \$ipl=0;$T";
  my $nc=$num_calls;
  $num_calls=$num_calls+2;
  $res.= "\narea$nc:;
my \$va=((split(\"\\n\",\$rlist.\"\\n\"))[\$ipl]);
if (\$va eq \"\") {goto area".($nc+1).";}
my \$vars=join(\"\\n\",split(\"\\t\",\$va));
$T";

  ###############################################################
  #Qui valorizzazione delle variabili
  ###############################################################
  my $vname; my $vn;
  foreach $vn(@variables)
  { $vname=((split(":",$vn))[0]);
    $res.= "if (varval(\"$vname\",\$vars) ne \"<Null>\") { \$EWB_$vname=varval(\"$vname\",\$vars); };\n";
  }
  if (($hp) && (substr($code, 0, 1) ne ")"))
  { errore "goal - parentesi non chiusa.";
  }
  if ($hp) { ($code,$rcd)=delperc(substr($code, 1)); }
  $res.= "$T";
#qui c'e' un ciclo.
  if (substr($code,0,1) ne "{") { errore "Necessaria \"{\" dopo goal. "; }

  inblock();
  $code=substr($code, 1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  while ( substr($code, 0, 1) ne "}")
  {
    my $rb;
    ($code,$rb)=(codice($code));
    $res=$res.$rb;
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

    #$code=delperc(codice($code));
  }
  ($code,$rcd)=delperc(substr($code,1));

  $res.= "
  \$ipl++;$T".
  "goto area".$nc.";
area".($nc+1).":;
";
  outblock();
  return ($code, $res);
}
