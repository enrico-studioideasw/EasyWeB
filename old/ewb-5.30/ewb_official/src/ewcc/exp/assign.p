#Sostituire questa con una serie di funzioni in forma prenessa.
#per ora ho eexp cfr eexp
#deve diventare
#ax=eexp <-. lo deve fare eexp
#bx=ax
#ax=eexp <- ...
#ax=bx cfr ax
sub is_assign($)
{ #ho un assignamento se trovo "variabile delperc = (e non ==)"
  #oppure se trovo "variabile delperc [ "
  #da rivedere
  my $code=$_[0];
#printf "is assign [".substr($code, 0, 7)."]? ";
  my $res=0;
  my $rb;
  my $r="";
  my $a=substr($code, 0, 1);
#qui c'e'una variabile..
  if   ( ((uc($a) ge "A") && ( uc($a) le "Z")) || ($a eq "_") )
  { $r=$a;
    $res=0;
    while ( ((uc($a) ge "A") && ( uc($a) le "Z")) || ( (uc($a)ge "0")&&(uc($a) le "9") ) || ($a eq "_") )
    { $code=substr($code, 1);
      $a=substr($code, 0, 1);
      if (($a ne "=") && ($a ne "[")) { $r.=$a; };
    };
    #ora $r deve contenere una variabile esistente.
    $r=join(//, split(/ /, $r));


    my $v;
    my $found="";
    foreach $v(@variables)
    { if ( ((split(":", $v))[0]) eq $r)
      { $found="var";
      }
      };
    if ($found ne "")
    {
#printf " valida ";
      ($code, $rb)=delperc($code);
      $a=substr($code, 0, 1);
      if ($a eq "[")
      { #printf "array ";
         #devo valutare un espressione e poi vedere se c'e' ] e  un uguale.
         #se trovo " o [ devo scorrere stringhe.
         $code=substr($code, 1);
         my $dabuttare;
         ($code, $dabuttare)=calcexp($code);
         ($code, $dabuttare)=delperc($code);
         if (substr($code, 0, 1) eq "]")
         { $code=substr($code, 1);
           ($code, $dabuttare)=delperc($code);
           if ((substr($code, 0,1) eq "=") && (substr($code,1,1) ne "=")) { $res=1; };
         };
      }
      else
      { # printf "semplice ";
        if ((substr($code, 0, 1) eq "=") && (substr($code, 1, 1) ne "="))
        {$res=1;} else {$res=0;};
      };
    };
  };
# printf "is assign: ".$res."\n";
  return $res;
};

sub assign($)
{ my $code=$_[0];
  my $vn;
if ($dbg) {printf "assign ".substr($code, 0, 7)."\n";};

  my $res="";
  #variable torna il valore della variabile, e $code tagliato.
  ($code, $vn)=variable($code);
  my $rt; 
  ($code, $res)=delperc($code);

#la var. deve esistere in ambiente. 

    my $v;
    my $found="";
    foreach $v(@variables)
    { if ( ((split(":", $v))[0]) eq $vn)
      { $found="var";
      }
    };
    if ($found eq "") { errore "Variabile inesistente $vn\n"; };



  if (substr($code, 0, 1) ne "[")
  { #normale variabile. 

    #valuto inexp
    #var=$a
    if (substr($code, 0,1) ne "=") 
    { errore "Errore interno in assign. scusate."; };
    ($code,$rt)=delperc(substr($code,1)); $res.=$rt; 
    ($code,$rt)=inexp($code);$res.=$rt; 
    $res.="\$EWB_".$vn."=\$ax;"; 
  }
  else
  { 
    ($code,$rt)=delperc(substr($code,1));
    $res.=$rt; 
    ($code,$rt)=calcexp($code);
    $res.=$rt; 
    ($code,$rt)=delperc($code);
    $res.=$rt; 
    $res.="push \@as, \$ax;$T";
    #tolgo "]"
    if (substr($code,0,1) ne "]")
    { errore "errore di sintassi: mi aspettavo chiusa quadra"; 
    };
    $code=substr($code,1);
    ($code,$rt)=delperc($code);
    $res.=$rt;
#qui togliere l'uguale
    if ((substr($code, 0, 1) ne "=") || (substr($code, 1, 1) eq "="))
    { errore "errore di sintassi: mi aspettavo un uguale";
    };
    $code=substr($code,1);
    ($code,$rt)=delperc($code);
    $res.=$rt;

 
    ($code,$rt)=calcexp($code);
    $res.=$rt; 
    ($code,$rt)=delperc($code);
    $res.=$rt; 
    $res.="\$bx=pop \@as;$T"."setelem(\$EWB_".$vn.", \$bx,\$ax);$T";    
    
    #var[calcexp]=inexp
    #valuto calcexp 
    #push $a
    #valuto inexp
    #$b=pop 
    #setelem(var, $b, $a)
  };  
if ($dbg) {printf "assign ret ".substr($code, 0,7)."\n"; }; 

  return ($code, $res); 
};

