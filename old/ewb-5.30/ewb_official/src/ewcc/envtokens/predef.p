sub predeff($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);
  my ($tmp,$a)=variable($code);
#print "#db predef on $a\n";
  if ($a eq "print")
  { ($code,$res)=PD_print($code);
  }
  elsif ($a eq "eprint")
  { ($code,$res)=PD_eprint($code);
  }
  elsif ($a eq "input")
  { ($code,$res)=PD_input($code);
  }
  elsif ($a eq "int")
  { ($code,$res)=PD_int($code);
  }
  elsif ($a eq "abs")
  { ($code,$res)=PD_abs($code);
  }
  elsif ($a eq "cos")
  { ($code,$res)=PD_cos($code);
  }
  elsif ($a eq "sin")
  { ($code,$res)=PD_sin($code);
  }
  elsif ($a eq "hex")
  { ($code,$res)=PD_hex($code);
  }
  elsif ($a eq "sqr")
  { ($code,$res)=PD_sqr($code);
  }
  elsif ($a eq "random")
  { ($code,$res)=PD_random($code);
  }
  elsif ($a eq "mid")
  { ($code,$res)=PD_mid($code);
  }
  elsif ($a eq "index")
  { ($code,$res)=PD_index($code);
  }
  elsif ($a eq "change")
  { ($code,$res)=PD_change($code);
  }
  elsif ($a eq "split")
  { ($code,$res)=PD_split($code);
  }
  elsif ($a eq "join")
  { ($code,$res)=PD_join($code);
  }
  elsif ($a eq "len")
  { ($code,$res)=PD_len($code);
  }
  elsif ($a eq "uc")
  { ($code,$res)=PD_uc($code);
  }
   elsif ($a eq "asc")
  { ($code,$res)=PD_asc($code);
  }
  elsif ($a eq "char")
  { ($code,$res)=PD_char($code);
  }
  elsif ($a eq "load")
  { ($code,$res)=PD_load($code);
  }
  elsif ($a eq "loaddir")
  { ($code,$res)=PD_loaddir($code);
  }
  elsif ($a eq "save")
  { ($code,$res)=PD_save($code);
  }
  elsif ($a eq "push")
  { ($code,$res)=PD_push($code);
  }
  elsif ($a eq "pop")
  { ($code,$res)=PD_pop($code);
  }
  elsif ($a eq "getelem")
  { ($code,$res)=PD_getelem($code);
  }
  elsif ($a eq "setelem")
  { ($code,$res)=PD_setelem($code);
  }
  elsif ($a eq "numel")
  { ($code,$res)=PD_numel($code);
  }
  elsif ($a eq "return")
  { ($code,$res)=PD_return($code);
  }
  elsif ($a eq "end")
  { ($code,$res)=PD_end($code);
  }
  elsif ($a eq "date")
  { ($code,$res)=PD_date($code);
  }
  elsif ($a eq "time")
  { ($code,$res)=PD_time($code);
  }
  elsif ($a eq "crypt")
  { ($code,$res)=PD_crypt($code);
  }
  elsif ($a eq "lessdate")
  { ($code,$res)=PD_lessdate($code);
  }
  elsif ($a eq "getenvvar") #aggiunta dalla 2.10
  { ($code,$res)=PD_GetEnvVar($code);
  }
  elsif ($a eq "getremoteaddress") #aggiunta dalla 2.10
  { ($code,$res)=PD_GetRemoteAddress($code);
  }
  elsif ($a eq "getquerystring") #aggiunta dalla 2.10
  { ($code,$res)=PD_GetQueryString($code);
  }
  elsif ($a eq "exec")
  { ($code,$res)=PD_exec($code);
  }
  else {
  ($code,$res)=tcpconnect($code);
  };
  #gestione di tabella di database.
  if (substr($code, 0, 1) eq ";") {
  #$res=$res. ";\n";
  my $t; ($code,$t) =delperc(substr($code, 1));  }

  return ($code,$res);
}
