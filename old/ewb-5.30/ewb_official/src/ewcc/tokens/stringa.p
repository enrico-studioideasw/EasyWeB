sub is_strvalue($)
{ my $code=$_[0];
  my $r;
  ($code, $r)=delperc($code);
  return (
    (substr($code, 0, 1) eq "\"" ) ||
    (substr($code, 0, 1) eq "[" ));
}
#*** va bene cosi'
sub strvalue($)
{
  my $res = "";
  my $code=$_[0];
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  if (substr($code, 0,1) eq "\"")
  { #stringa con virgolette. Ammette \"
    $res=$res. "\"";
    $code=substr($code, 1);
    do
    { if (substr($code, 0, 2) eq "\\\"")
     {  $res=$res. "\\\""; $code=substr($code, 2); }
      if (substr($code, 0, 1) ne "\"")
     {  $res=$res. substr($code, 0, 1); $code=substr($code, 1); }
    }
    while(($code ne "") && (substr($code, 0, 1) ne "\""));
    if ($code eq "") { errore "stringa non terminata."; };
    $res=$res. "\"";
    $code=substr($code, 1);
  }
  elsif (substr($code, 0,1) eq "[")
  { #stringa con quadre. Ammette \] e "
    $res=$res. "\"";
    $code=substr($code, 1);
    do
    {
      if (substr($code, 0, 2) eq "\\\]")
      {  $res=$res. "]"; $code=substr($code, 2); }
      if (substr($code, 0, 1) eq "\"")
      {  $res=$res. "\\\""; $code=substr($code, 1); }
      elsif (substr($code, 0, 1) ne "]")
      {  $res=$res. substr($code, 0, 1); $code=substr($code, 1); };
    }
    while(($code ne "") && (substr($code, 0, 1) ne "]"));
    if ($code eq "") { errore "stringa non terminata."; }
    $res=$res. "\"";
    $code=substr($code, 1);
  }
  else
  { errore "mi aspettavo una stringa.";
  }
  return ($code,$res);
}
