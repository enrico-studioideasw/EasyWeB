sub database($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);
  my ($tmp,$a)=variable($code);
  if ($a eq "lock")
  { 		#ok - porting alla 2.0
    ($code,$res) =PD_lock($code);
  }
  elsif ($a eq "unlock")
  { 		#ok - porting alla 2.0
    ($code,$res)=PD_unlock($code);
  }
  elsif ($a eq "add")
  { 		#ok - porting alla 2.0
    ($code,$res)=PD_add($code);
  }
  elsif ($a eq "update")
  {

printf "update.. inizio codice.\n";

    ($code,$res)=PD_update($code);
  }
  elsif ($a eq "exist")
  { 		#ok - porting alla 2.0
    ($code,$res)=PD_exist($code);
  }
  elsif ($a eq "login")
  { 		#ok - porting alla 2.0
    ($code,$res)=PD_login($code);
  }
  elsif ($a eq "remove")
  { 		#ok.. a posto nella 2.0
    ($code,$res)=PD_remove($code);
  }
  elsif ($a eq "sort")
  { 		#ok - porting alla 2.0
    ($code,$res)=PD_sort($code);
  }
  elsif ($a eq "loadtab")
  {
    ($code,$res)=PD_loadtab($code);
  }
  elsif ($a eq "savetab")
  {
    ($code,$res)=PD_savetab($code);
  }
  elsif ($a eq "in")
  { 		#ok - porting alla 2.0
    ($code,$res)=PD_in($code);
  }
  elsif ($a eq "inall")
  { ($code,$res)=PD_inall($code);
  }
  elsif ($a eq "del")
  { #analoga alla inall ma cancella i campi che rispettano la condizione.
    ($code,$res)=PD_del($code);
  }
#gestione di funzionalita' di web asking.
  else
  {
    ($code,$res)=webask($code);
    my $t; ($code, $t)=delperc($code);
  }
  return ($code,$res);
}
