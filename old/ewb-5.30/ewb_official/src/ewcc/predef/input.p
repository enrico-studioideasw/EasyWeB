sub PD_input($)
{ my $code=$_[0];
  my $res="";
  $code=substr($code, 5);
  my $t; ($code,$t)=delperc $code;
  if (! is_identifier($code))
  { $res=$res."\$ax=(<>);$T";
    #errore "Input si usa input variabile"; 
  }
  else
  {
  my $v;
  ($code, $v)= variable($code);
  $res=$res. "(\$EWB_".$v;
  $res=$res. "=(<>));$T";
  };
  return ($code,$res);
}
