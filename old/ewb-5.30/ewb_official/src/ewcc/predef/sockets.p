sub tcpconnect($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);
  my ($tmp, $a)=variable($code);
  if ($a eq "socket")
  { ($code,$res)=PD_socket($code);
  }
  elsif ($a eq "server")
  { ($code,$res)=PD_server($code);
  }
  elsif ($a eq "accept")
  { ($code,$res)=PD_accept($code);
  }
  elsif ($a eq "sread")
  { ($code,$res)=PD_sread($code);
  }
  elsif ($a eq "swrite")
  { ($code,$res)=PD_swrite($code);
  }
  elsif ($a eq "close")
  { ($code,$res)=PD_close($code);
  }
  else
  { 
    ($code,$res)=database($code);
    my $t; ($code, $t)=delperc($code);
  }
  return ($code,$res);
}
