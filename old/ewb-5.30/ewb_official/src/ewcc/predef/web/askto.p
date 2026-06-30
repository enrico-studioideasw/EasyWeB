sub PD_askto($code)
{ #la funzione che genera un campo per il form.
  #i campi sono visualizzati in tabella e centrati.
  #un campo e' una riga della tabella.
  #form field, field
  my $code=$_[0];
  my $dest="";
  my $res="";
  my $rr;
  ($code,$rr)=delperc(substr($code, 5));

  #leggo l' url e lo metto in $dest.
  ($code,$dest)=geturl($code);
  #mangio la virgola.
  if (substr($code,0,1) ne ",")
  {errore "askto: mi aspetto una virgola dopo url.";
  }
  ($code,$rr)=delperc(substr($code,1));

################da qui e' analoga alla ask. tranne che per la destinazione.
  $res.= "my \$form=\"\";$T";

  #my $code=delperc(PD_field($code));
  my $rb;
  ($code,$rb)=(PD_field($code));
  $res=$res.$rb;
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  $res.= "\$form.=CreateForm(\$ax,\$bx,\$cx,\$dx);$T";

  while (substr($code, 0, 1) eq ",")
  { ($code,$rr)=delperc(substr($code,1));

    #$code=delperc(PD_field($code));
    my $rb;
    ($code,$rb)=(PD_field($code));
    $res=$res.$rb;
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
    $res.= "\$form.=CreateForm(\$ax,\$bx,\$cx,\$dx);$T";

  }
  my $n;
  foreach $n(@variables)
  { #se la variabile non appare in form la aggiungo come hidden.
    $n=((split(":", $n))[0]);

    $res.= "
    if (index(\$form, \"name=\\\"$n\\\"\") <0)
     { \$form=\$form.CreateForm(\"$n\", \$EWB_$n, \"hidden\");
     } ";
  }
  #$res.= "\n\$form=\$form.CreateForm(\"EWjump\", $num_labels+1, \"hidden\"); \n";
  $res.= "\n\$form=\$form.CreateForm(\"EWjump\", 0, \"hidden\"); \n";
  $res.= "\n\$form=\$form.CreateForm(\"___from_an_ask_to\", \"FROMASKTO\", \"hidden\"); \n";

  $res.= "\n\$form=\$form.\"<input type=hidden name=\\\"EWB_Hid_FIELD\\\" value=\\\"".(rand(1))."\\\">\\\n\";\n";

  #faccio scrivere il form contornato dal template.
  { $res.= "\$form=\"<form action=\\\"$dest\\\" method=post enctype=\\\"multipart/form-data\\\">\".\$form.\"</form>\";\n";
  }
  $res.= "print join(\$form,split(\"<>\", \$EWB_template)); exit(0);$T";
  #indico sul file il punto di arrivo di una label.
  $res.=PD_addlabel();
  return ($code,$res);
}
