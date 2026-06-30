#questa la riscrivo.
sub PD_form($code)
{ #la funzione che genera un campo per il form.
  #i campi sono visualizzati in tabella e centrati.
  #un campo e' una riga della tabella.
  #form field, field
  my $code=$_[0];
  my $rr;
  my $res; 
  ($code,$rr)=delperc(substr($code, 4));

  my ($code,$rr)=delperc($code);
  my $rb;
  ($code,$rb)=(PD_field($code));
  $res.=$rb; 
  my $cnt=0;
  $res.="\$ax=CreateForm(\$ax,\$bx,\$cx,\$dx);";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

  while (substr($code, 0, 1) eq ",")
  {
    ($code,$rr)=delperc(substr($code,1));
    my $rb;
    ($code,$rb)=(PD_field($code));
    $res.="push \@as,\$ax;$T";
    $res.=$rb;
    $res.="\$ax=CreateForm(\$ax,\$bx,\$cx,\$dx);";
    $cnt++;
    my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
    $res.="\$ax=(pop \@as).\$ax;$T";
  };
  return ($code,$res);
}
