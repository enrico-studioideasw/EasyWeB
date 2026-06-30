sub PD_showform($)
{ #qui la vita si complica. chiamo una funzione nativa.
  #devo mostrare a video il dato passato come parametro secondo il template predefinito.
  #devo aggiungerci pero' le var. che non appaiono nel form.
  #trucco: mi cerco i "name=\"" e vedo quali var. dichiarate in ambiente gia' ci sono.
  #nota: al ritorno di una ask potrei perdere le non globali coperte..
  #la var. di showform viene vuotata.
  my ($code)=@_[0];
  my @names;
  my $res="";
  my $rr;
  ($code,$rr)=delperc(substr($code,8));
  ($code, $names[0])=variable($code);

  $res.= "{ my \$form=";
  while (substr($code, 0, 1) eq ",")
  { $code=substr($code, 1);
    ($code,@names[$#names+1])=variable($code);
  }
  my $n;
  foreach $n(@names)
  { $res.= "\$EWB_".$n.".";
  }
  $res.= "\"\";\n";
  foreach $n(@names)
  { $res.= "\$EWB_".$n."=\"\"; ";
  }
  foreach $n(@variables)
  { #se la variabile non appare in form la aggiungo come hidden.
    $n=((split(":",$n))[0]);
    $res.="
    if (index(\$form, \"name=\\\"$n\\\"\") <0)
     { \$form=\$form.CreateForm(\"$n\", \$EWB_$n, \"hidden\");
     };";
  }
  $res.= "\n\$form=\$form.CreateForm(\"EWjump\", $num_labels+1, \"hidden\"); \n";
  #aggiungo un campo random.
  $res.= "\n\$form=\$form.\"<input type=hidden name=\\\"EWB_Hid_FIELD\\\" value=\\\"".(rand(1))."\\\">\\\n\";\n";

  #faccio scrivere il form contornato dal template.
  { $res.= "\$form=\"<form name=\\\"ewb\\\" action=\".\$PROGRAM_NAME.\" method=post enctype=\\\"multipart/form-data\\\">\".\$form.\"</form>\";\n";
  }
  $res.= "print join(\$form,split(\"<>\", \$EWB_template)).\"\\\n\\\n\"; exit(0); \n}\n";
  #indico sul file il punto di arrivo di una label.
  $res.=PD_addlabel();
  return ($code,$res);
}
