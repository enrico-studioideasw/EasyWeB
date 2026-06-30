sub programma($)
{ my $code=$_[0];
  my $res;
  while ($code ne "")
  { my $r="";
    ($code, $r)=codice($code);
    $res.=$r;
  }

#roba predefinita per macchina virtuale.
#accumulatori, stack degli accumulatori,
$res="my \$ax,\$bx,\$cx,\$dx,\@as;\n".$res;

#-----   punti di accesso di ritorno da ask.
  $res.= "exit(0); \n
sub jumper(\$)
{ my \$lab=\$_[0];\n";
  my $i;
  for($i=1; $i<=$num_labels; $i++)
  { $res.= "if (\$lab==$i) { goto lab$i; }\n";
  }
  $res.= "};\n";
#-----
  return ($code, $res);
}

sub codice($)
{ my $code=$_[0];
  my $res="";
  my $r;
  if (substr($code, 0, 1) eq ";") {
  #$res.= ";\n";
  ($code,$r)=delperc(substr($code, 1));  }
  $res.=$r;

  if (is_commento($code))
  { my $r;
    ($code,$r)=commento($code);
    $res.=$r;
  }
  elsif ( (substr($code, 0, 5) eq "table") && isnottoken(substr($code, 5)) )
  { my $r;
    ($code,$r)=tabdecl($code);
    $res.=$r;
  }
  elsif ( (substr($code, 0, 3) eq "var") && isnottoken(substr($code, 3)) )
  { my $r;
    ($code,$r)=vardecl($code);
    $res.=$r;
  }
  elsif ( (substr($code, 0, 2) eq "if") && isnottoken(substr($code, 2)) )
  { my $r;
    ($code,$r)=ifblock($code);
    $res.=$r;
  }
  elsif ( (substr($code, 0, 5) eq "while") && isnottoken(substr($code, 5)) )
  { my $r;
    ($code,$r)=whileblock($code);
    $res.=$r;
  }
  elsif ( (substr($code, 0, 7) eq "foreach") && isnottoken(substr($code, 7)) )
  { my $r;
    ($code,$r)=foreachblock($code);
    $res.=$r;
  }
  elsif ( (substr($code, 0, 3) eq "for") && isnottoken(substr($code, 3)) )
  { my $r;
    ($code,$r)=forblock($code);
    $res.=$r;
  }
  elsif ( (substr($code, 0, 2) eq "do") && isnottoken(substr($code, 2)) )
  { my $r;
    ($code,$r)=downwhile($code);
    $res.=$r;
  }
  elsif ( (substr($code, 0, 3) eq "sub") && isnottoken(substr($code, 3)) )
  { my $r;
    ($code,$r)=fundecl($code);
    $res.=$r;
  }
  else
  { if (($code ne "") && (substr($code,0,1) ne "}"))
    { my $r;
      ($code,$r)=calcexp($code);
      $res.=$r;
    }
  }
  if (substr($code, 0, 1) eq ";") {
  ($code,$r)=delperc(substr($code, 1));  }
  $res.=$r;
  return ($code,$res);
}

#delperc:elimina i caratteri di spaziatura, conta le righe e le riporta su stdout per debug.
sub isnottoken($)
{ my $code=$_[0];
  my $a;
  $a=substr($code, 0, 1);
  return (!( ((uc($a) ge "A") && ( uc($a) le "Z")) || ($a eq "_") || (($a ge "0")&&($a le "9"))));
}

sub isperc($)
{ my $code=$_[0];
  my $a;
  $a=substr($code, 0, 1);
  return (($a eq ' ')|| ($a eq "\t") || ($a eq "\n") );
}
my @linenums;
my $currfile=$ARGV[0];
#***
sub delperc($)
{ my $code=$_[0];
  my $res;
  my $a;
  do
  { $a=substr($code, 0, 1);
    if (($a eq " ") || ($a eq "\t") || ($a eq "\n"))
    { $code=substr($code, 1);
      if ($a eq "\n") { $curr_row=(split("\n", $code))[0];
                        if (substr($curr_row,0,5) eq "'F_NM")
		        { push @linenums,$currfile;
			  push @linenums,$num_row;
		          $currfile=substr($curr_row,5);
			  $code=substr($code, length($curr_row)+1);
			  $curr_row=(split("\n", $code))[0];
                          $num_row=0;
			}
			if (substr($curr_row,0,5) eq "'FEND")
			{ $num_row=pop @linenums;
			  $currfile=pop @linenums;
			  $code=substr($code, length($curr_row)+1);
			  $curr_row=(split("\n", $code))[0];
			}
			$num_row++;
			# modifica 030424P inizio
			# aggiungo il numero di riga del sorgente per ritrovarmi meglio
			#if ($debugmode) { $res.= "\n###".$curr_row.$T; };
			if ($debugmode) { $res.= "\n###".$num_row."#".$curr_row.$T; };
			# modifica 030424P fine
		      };
      $a = "jump";
    }
  } while ($a eq "jump");
  return ($code,$res);
}

#***
sub errore($)
{ my $err=$_[0];
  my $a=$ARGV[0]; if ($a eq "") { $a="STDIN"; }
  print stderr "Errore alla linea $num_row\n$curr_row \n$a: $err\n";
  print "\n#EWB: errore\n";
  exit(-1);
}
#commento: token di commento - inizia con ' e finisce a fine riga.
sub is_commento($) {my $code=$_[0];
  return ((substr $code, 0, 1) eq "\'");};
#***
sub commento($)
{ my $code=$_[0];
  my $res="";
  while ((substr($code, 0,1) ne "\n") && ($code ne ""))
  { $res.=substr($code,1);
    $code=substr($code, 1, length($code));
  }
  $res.="\n";
  my ($c,$r)=delperc($code);
  return ($c,$r);
}
