#!/usr/bin/perl
my @EWlocked;
use IO::Socket;
my @fltype;
my $EWB_tabpath="./";
my $EWB_template="<html><body bgcolor=lightyellow><div align=center><></body></html>";
my $EWB_format="12:5:h";
my $EWB_sqldbase="";
my $EWB_sqlserver="";
srand(time());
my $PROGRAM_NAME;
{ my @a=split('/', $0);
  $PROGRAM_NAME=$a[$#a];
}
my $rlist="";

my $ISASKTO = "";
#gestione di forms multipart encoded
sub read_form_buffer()
{ my $b=$ENV{"CONTENT_LENGTH"};
  my $all=""; my $i;
  for ($i=0; $i<$b;)
  { $all=$all.<>;
    $i=length($all);
  };
  #pesco il boundary e ricostruisco, per quanto possibile, la query string.
  my $boundary=((split("\n", $all))[0]);
  my @all=split($boundary, $all);
  #se la prima riga termina con "name=", lo metto in query string
  $i=0;
  foreach $p(@all)
  {
    my ($a, $first)=(split("\n", $p));
    ($a, $b)=split(" name=\"", $first);
    if ($b ne "")
    { my $name;
      ($name, $b)=split("\"", $b);
      $parname[$i]=$name;
      #il value e' nella quarta riga.
      if (index($first, " filename=\"")<0)
      { my $rest;
          $rest="";
          for($k=4; $k<=(split("\n", $p)); $k++)
          { $rest=$rest.( (split("\n", $p))[$k]);
          };
        if ( ($rest) eq "")
        {
          $parvalue[$i]=((split("\n", $p))[3]);
	  $parvalue[$i]=substr($parvalue[$i], 0, length($parvalue[$i])-1);
	  $fltype[$i]="text";
          if (substr($parvalue[$i],0, length("HidDenFieldDaTa")) eq
                  ("HidDenFieldDaTa"))
          { $parvalue[$i]=fromasc(substr($parvalue[$i],length("HidDenFieldDaTa")));
          };
        }
        else
        { my $k;
          $parvalue[$i]="";
          for($k=3; $k<=(split("\n", $p)); $k++)
          { $parvalue[$i]=$parvalue[$i].( (split("\n", $p))[$k])."\n";
          };
	  $parvalue[$i]=substr($parvalue[$i], 0, length($parvalue[$i])-2);
        };
        $i++;

      } else
      { #cerco il content_type
        $fltype[$i]="file";
        my $ctype=((split("\n", $p))[2]);
	$ctype=substr($ctype, 0, length($ctype)-1);
	#print "Ctype: [$ctype]\n";
        $parvalue[$i] = "";
	if (substr(uc($ctype), 0, length("Content-type: "))  eq "CONTENT-TYPE: ")
	{ #trasferimento file di testo.
	  my @p=split("\n", $p);
	  $p[0]=""; $p[1]=""; $p[2]=""; $p[3]="";
	  $partype[$i]=substr($ctype, length("Content-type: "));
	  #printf("partype: $partype[$i]\n");
	  $parvalue[$i]=join("\n", @p);
	  $parvalue[$i]=substr($parvalue[$i], 4);
          $parvalue[$i]=substr($parvalue[$i], 0, (length($parvalue[$i])-1));
	}
        $i++;
      }
    }
  }

  #se ci sono parametri con nome  replicato, diventano un'unico parametro con
  #valori separati da ;
  my  $p, $r, $fn; my $cnp=0;
  my @pnm; my  @pvl;
  foreach $p(@parname)
  { $fn=0;
    my $cni=0;
    foreach $r(@pnm)
    { if  ($p eq $r)
       { $pvl[$cni]=$pvl[$cni].";".$parvalue[$cnp];
	 $fn=1;
       };
       $cni++;
    };
    if ( $fn==0 )
    { $pnm[$#pnm+1]=$p;
      $pvl[$#pvl+1]=$parvalue[$cnp];
    };
    $cnp++;
  };
  @parname=@pnm;
  @parvalue=@pvl;

  #qui mostro tutti i parametri. Rielaboro quelli corrispondenti a multicheck
  # e piu'avanti anche a multilist.
  my $p; my $cnt=0;
  my  @pv; my @tohide; my  @hideval;
  foreach $p(@parname)
  { if (substr($p, 0, length("EWcHECK_")) eq  "EWcHECK_")
    { $p=substr($p, length("EWcHECK_"));
      my $n=int($p);
      $p=substr($p, length($n));
#      print "Par ".$p." vale ".$parvalue[$cnt]."<br>";

      my $fnd=0;
      for ($n=0; $n<=$#tohide; $n++)
      { if ($p eq $tohide[$n])
        { $fnd=1;
          $hideval[$n]=$hideval[$n].";".$parvalue[$cnt];
        };
      };
      if ($fnd!=1)
      { $n=$#tohide+1;
        $tohide[$n]=$p;
        $hideval[$n]=$parvalue[$cnt];
      };
    };
    $cnt++;
  };
  $cnt=0;
  foreach $p(@tohide)
  { my $a;
    my $ctb=0;
    foreach $a(@parname)
    { if ($a eq $p) { $parvalue[$ctb]=$hideval[$cnt]; }
      $ctb++;
    }
    $cnt++;
  };
  if (get_par("___from_an_ask_to") ne "")
  {
    $ISASKTO = get_par("___from_an_ask_to");
  }
}
