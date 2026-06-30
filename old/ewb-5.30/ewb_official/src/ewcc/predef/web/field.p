sub PD_field($)
{ #legge un campo da $code e la sua specifica,
  #scrive nomecampo, valorecampo, tipocampo per createform.
  #poi devo definire createform. Non ammette ".
  #mette i parametri in $ax, $bx, $cx, $dx

  my $code=$_[0];
  my $type="text";
  my $res="";
  my $name=variable($code);
  my $coso="\"$name\"";

#qui aggiungere un controllo: il nome campo deve esistere in ambiente 
#print "Campo $coso\n";
  my $fnd=0; my $ln; 
  foreach $ln(@variables)
  { $ln=((split(":", $ln))[0]); 
    if ($ln eq $name) 
    {  $fnd=1; 
    };
  };
  if ($fnd==0) { errore "Variabile ".$coso." sconosciuta in richiesta ask.";};

  my $rr; ($code,$rr)=delperc(substr($code, length($name)));


  if (substr($code, 0, 1) eq ":")
  { #e' stato specificato un tipo
    ($code,$rr)=delperc(substr($code,1));
    $type=variable($code);

    ($code,$rr)=delperc(substr($code, length($type)));
    if ( ($type ne "text") && ($type ne "password") &&
    ($type ne "submit") &&($type ne "textarea") &&
    ($type ne "links") &&($type ne "option") &&
    ($type ne "checkbox") &&($type ne "radio") &&
    ($type ne "hidden") && ($type ne "info") && 
    ($type ne "checks" )&& ($type ne "options") &&
    ($type ne "file" )&& ($type ne ""))
    { errore "Campo a video:specificato tipo sconosciuto.";
    }
    if ($type eq "") {$type="text"; };
#####################################
#dopo il tipo puo' essere specificato il testo di apertura tra parentesi.
#####################################

    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
    if (substr($code,0,1) eq "(")
    {
      $code=substr($code,1);
      my $rb;
      ($code,$rb)=(calcexp($code));
      $res=$res.$rb;
      my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;

      if (substr($code,0,1) ne ")")
      { errore "Form: Il campo prevede un solo nome. ";
      };
      $coso= "\$ax";
      ($code,$rr)=delperc(substr($code,1));
    }else
    {
      #se ho un bottone e non ho specificato testo, il testo non ci va
      if ( ($type eq "submit") )
      {
        $coso="\"\"";
      };
    };
  } else
  {
  };
  $res.= "push \@as,\"$name\";$T"."push \@as,\$EWB_$name;$T"."push \@as,\"$type\";$T";
  $res.= "push \@as,$coso;$T";
      $res.= "\$dx=pop \@as;$T";
      $res.= "\$cx=pop \@as;$T";
      $res.= "\$bx=pop \@as;$T";
      $res.= "\$ax=pop \@as;$T";
  return ($code, $res);
}
