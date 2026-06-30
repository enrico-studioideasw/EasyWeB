############ Sezione di linker. 

#versione 3.07: c'e' un errore e il codice ewb viene riportato due volte nel compilato. 


sub lnerror($)
{ print "EWLN: $_[0]\n";
  exit -1;
};

sub lnmain($$)
{ my ($code,$file)=@_;
  #print $code."\n";
  my $toadd="";
  if (! (open iiif,"< /lib/ewb/$file"))
  { lnerror "libreria /lib/ewb/$file non trovata.";
  }
  my $lbr=join("",<iiif>);
  close iiif;
  my $i=0;
  my $found=0;
  my $fuc=0;
  my @lb=split("\nsub ",$lbr);
  $toadd.=$lb[0];
  my $userc;
  #scorro tutto questo.
  #se trovo una funzione presente su $code la aggiungo a $toadd
  #e la tolgo da qui
  #se trovo usercode; lo copio e lo tolgo da qui. 
  do
  { $found=0;
    my $allcode=$code.$toadd;
    for ($i=1; $i<=$#lb; $i++)
    { $userc="";
#print substr($lb[$i],0,7)."\n";
      $name=lngetname($lb[$i]);
#print $name."\n";
      if (($fuc==0)&&(index($lb[$i],"#<EWB_usercode>\n")>0))
      { #print "Trovato usercode\n";
        $userc="\nEWB_EWLN_usercode;\n\n";
        $fuc=1;
      };
      if (($name ne "")&(index($allcode,$name)>0))
      { print "ins. funzione $name\n";
        $toadd.="\nsub ".$lb[$i];
        $lb[$i]="";
        $found=1;
      };
      $toadd=$toadd.$userc;
    };
    #print "end loop: $found\n";
  } while ($found==1); 
  
  
  $toadd=join("\n".$code."\n",split("\nEWB_EWLN_usercode;\n",$toadd));
  #print $toadd; exit;
  return $toadd;
};

sub lngetname($)
{ #torna il nome della variabile. Adattato al nuovo modo.
  my $code=$_[0];
  my $name="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my $a=substr($code, 0, 1);
  if ( ((uc($a) ge "A") && ( uc($a) le "Z")) || ($a eq "_"))
  { $name=$name.$a;
    $code=substr($code, 1);
    $a=substr($code, 0, 1);
  }
  while ( ((uc($a) ge "A") && ( uc($a) le "Z")) || ($a eq "_") || (($a ge "0")&&($a le "9")))
  { $name=$name.$a;
    $code=substr($code, 1);
    $a=substr($code, 0, 1);
  }
  return ($code, $name);
}
