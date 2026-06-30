#!/usr/bin/perl
############ Sezione di preprocessore. 
my $curr_row=1;
my $num_row=1;

sub pperror($)
{ print "EWPP: $curr_file Line \"$curr_row\"\n $_[0]\n";
  exit -1;
};


sub ppmain($)
{ $code=$_[0];

#if ($ARGV[1] eq "")
#{ pperror "Uso ewpp inputfile outputfile.";
#};

#open iif, "<$ARGV[0]" ;
#  $curr_file = $ARGV[0];
#  $code=join("", <iif>);
#close iif;

#se la prima riga e' #! la scarto.
if (substr($code, 0, 2) eq "#!") {$code="'".$code; $num_row=1;};
  $code=preprocessore($code);
#open oof, ">$ARGV[1]";
#print oof $code;
#close oof;
#exit 0;
  return $code;
};


#___________________________________________________________________
#predicato di include... per le librerie.  Preprocessore.
#prima di programma, come preprocessore, analogo a quello c.

sub preprocessore($)
{ my $code=$_[0];
  my @defines;
  my @defs;

  my $path="/usr/lib/ewb/";

  my @code=split("\n", $code);
  my $i=0;
  my $l;
  my $commode=0;
  #sostituisco current_line senno' sballo il debuching.
  my $cr=$curr_row;
  my $nr=$num_row;
  foreach $l(@code)
  {
    $num_row=$i+1;
    $curr_row=$l;

#primitiva #define
    if (substr(ppdelperc($l), 0, 7) eq "#define")
    { #Qui non cambio il numero di riga.
      #mi limito a memorizzare la define.
      $l=ppdelperc(substr(ppdelperc($l),7));
      my $name=((split(/ /, $l))[0]);
      $l=ppdelperc(substr($l, length($name)));
      my $val=$l;
      $defs[$#defs+1]=$name."#".$val;
      $l="";
      $code[$i]="";
      #poi il codice verra' splitjoinato per le defs.
    }
#primitiva ifdef
    if (substr(ppdelperc($l), 0, 6) eq "#ifdef")
    { #vediamo se la var. e' definita.
      $l=substr(ppdelperc($l),6);
      $l=ppdelperc($l);
      my $name=((split(/ /, $l))[0]);
      $commode=1;
      my $d;
      foreach $d(@defs)
      {
        if (((split("#", $d, ))[0]) eq $name) { $commode=0; }
      }
      $l="";
      $code[$i]="";
    }
    if (substr(ppdelperc($l), 0, 7) eq "#ifndef")
    { #vediamo se la var. e' definita.
      $l=substr(ppdelperc($l),7);
      $l=ppdelperc($l);
      my $name=((split(/ /, $l))[0]);
      $commode=0;
      my $d;
      foreach $d(@defs)
      { if (((split("#",$d ))[0]) eq $name) { $commode=1; }
      }
      $l="";
      $code[$i]="";
    }
    if (substr(ppdelperc($l), 0, 6) eq "#endif")
    {$commode=0;
      $l="";
      $code[$i]="";

    };

    if ($commode) { $l=""; $code[$i]="' ".$l; }

#primitiva #include

    if (substr(ppdelperc($l), 0, 8) eq "#include")
    { #linea di include di preprocessore.
      #posso gestire #include, #define, #ifdef, #endif

      $l=ppdelperc(substr(ppdelperc($l), 8));

      my $def="";

      if (substr($l, 0, 1) eq "<")
      { $def=$path;
        my ($fn, $rest)=split(">", substr($l,1));
        if (ppdelperc($rest) ne "")
        { pperror "Primitiva di preprocessore include errata.";
        }
        $def=$def.$fn;
        $def=join("/", split("//", $def));
      }
      elsif (substr($l, 0, 1) eq "\"")
      { $def="";
        my ($fn, $rest)=split("\"", substr($l,1));
        if (ppdelperc($rest) ne "")
        { pperror "Primitiva di preprocessore include errata.";
        }
        $def=$def.$fn;
      }
      else
      { pperror "Primitiva di preprocessore include errata.";
      }

      my $found=0;
      my $d;
      foreach $d(@defines)
      { if ($d eq $def)
        { $found=1;
          #file gia' incluso.
        }
      }

      if (!$found)
      { $defines[$#defines+1]=$def;
	if (! open (iif, "< ".$def))
        { pperror "file di include non trovato: ".$def;
        }

#devo trovare modo di non sballare i numeri di riga.
#ci inserisco un 'F_NM:filename , 'FEND
#devo tenerne presente nella delperc.

        $l=""; $code[$i]="'F_NM:$def\n". join("\n", <iif>)."\n'FEND\n";

        close iif;
      };
      $i++;
    }
  }
  $code=join("\n",@code);

  #uso delle define.
  my $a;
  foreach $a(@defs)
  { my ($b, $c)=split("#", $a);
#    print "splitjoin $b/$c\n";
    if ($b ne "")
    { $code=join( $c, split ($b, $code));
    }
  }
  $curr_row=$cr;
  $num_row=$nr;
  return $code;
}


#################################################################################################
#################################################################################################
#################################################################################################
#################################################################################################

#delperc:elimina i caratteri di spaziatura, conta le righe e le riporta su stdout per debug.
sub ppisperc($)
{ my $code=$_[0];
  my $a;
  $a=substr($code, 0, 1);
  return (($a eq ' ')|| ($a eq "\t") || ($a eq "\n") );
}
my @linenums;
my $currfile=$ARGV[0];
sub ppdelperc($)
{ my $code=$_[0];
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
			#print "\n##\t".$curr_row."\n";
		      };
      $a = "jump";
    }
  } while ($a eq "jump");
  return $code;
}

