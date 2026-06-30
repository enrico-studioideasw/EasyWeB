#***
sub fundecl($)
{ my $code=$_[0];
  my $res="";
  #per ora non cambio il meccanismo di passaggio parametri.
  # una cosa alla volta.
  my @vvars;

  #print "fundecl\n"; exit(0);

  $code=substr($code, 3);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
  my ($code, $name)=variable($code);

#attenzione: in perl i nomei di funzioni e variabili hanno EWB_ premesso.
  $res=$res. "goto ENDEWB".$name.";$T";
  $res=$res. "\nJMPEWB".$name.":$T";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;


  if(substr($code, 0, 1) ne "(")
  { errore "sono necessari parametri tra parentesi."; }
  $code=substr($code, 1);
  #qui entro in un blocco, e le var. sono locali.
  inblock();

  my $arity=0;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code, 0, 1) ne ')')
  {
    $code=",".$code;
    do
    { $code=substr($code, 1);
      my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
      my ($t,$a)=variable($code);
      push @vvars,$a;
      push @variables, $a.":v";
      $code=substr($code, length($a));
      my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
      #lo stack deve contenere l' indirizzo di ritorno e le variabili
      #passate alla funzione.
      $res=$res. "my \$EWB_$a = pop \@varstack;$T";
      ++$arity;
    } while (substr($code, 0, 1) eq ",");
  }
  #controllo che termini con chiusa parentesi.
  if(substr($code, 0, 1) ne ")")
  { errore "parametro di sub sconosciuto."; }
  #$res=$res. "\n";
  $code=substr($code,1);


  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  #print "code:[$code]\n";
  if (substr($code, 0, 1) eq "{")
  { #corpo della funzione
    $code=substr($code, 1);

    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
    while ( substr($code, 0, 1) ne "}")
    { my $rb;
      ($code,$rb)=codice($code);
      $res=$res.$rb;
      my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
    }
    $code=substr($code,1);

    #$res=$res. "}";
    #meccanismo di ritorno da funzione
    $res.="goto (pop \@varstack);$T";
    $res.= "\nENDEWB".$name.":$T";

  outblock();
  }
  #aggiungo in ambiente la funzione - non locale.
  $functions[$#functions+1]=$name.":".$arity;

  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code, 0, 1) ne ";")
  { errore "manca ; dopo la dichiarazione di funzione.";
  }
  $res=$res. ";$T";
  $code=substr($code, 1);
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  #print "function:\n$res"; exit;
  return ($code,$res);
}
