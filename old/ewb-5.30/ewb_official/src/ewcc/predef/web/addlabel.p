sub PD_addlabel()
{
  ++$num_labels;
  my $n;
  my $res="";
  $res.= "\ngoto retlab$num_labels;";
  $res.= "\nlab$num_labels:;\n";
  #recupero le variabili per riprendere l' esecuzione.
  foreach $n(@variables)
  { $res.= "\$EWB_$n=get_par(\"$n\"); \n";
  }
  $res.= "retlab$num_labels:;\n";
  return $res;
}
