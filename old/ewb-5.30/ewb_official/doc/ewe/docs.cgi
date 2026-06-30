#!/usr/bin/perl
print "Content-type: text/html\n\n"; 
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
}

sub get_par($)
{ my ($name)=@_;
  my $res="";
  my $n; my $i=0;
  foreach $n(@parname)
  { if ($name eq EWconv($n))
    { if ($fltype[$i] ne "file") { $res=EWconv($parvalue[$i]); }
      else { $res=$parvalue[$i]; };
    };
    $i++;
  };
  return $res;
};

#funzioni predefinite
sub EWload($)
{ open iif,"<".$_[0];
  my $a=join("",<iif>);
  close iif;
  return $a;
}

sub EWgetenvvar($)
{
  my $r;
  $r= $ENV{$_[0]};
  return $r;
}

#2.10 fine

my @parname;
my @parvalue;
srand(time());
read_form_buffer();
#jumper area
if (get_par("EWjump")>0)
{ jumper(get_par("EWjump"));
};
my $ax,$bx,$cx,$dx,@as;
$ax="pages/docs.html";
	$ax=EWload($ax);
	$EWB_template=$ax;
my $EWB_a;
	$ax=(EWgetenvvar("QUERY_STRING"));
	$EWB_a=$ax;$ax=$EWB_a;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
};

#Amethist easyweb v. 5.21 - easy 
#Enrico Betti e Paride Dominici



sub EWconv($)
{ my $a=$_[0];
  $a=join("%3C", split("%253C", $a));
#  $a=~ tr/+/ /;
  $a=~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C",hex($1))/eg;
  $a=join("
", split("%0A", $a));
  $a=join("	", split("%09", $a));
  $a=join("\"", split("%22", $a));
  $a=join("<", split("%3C", $a));
  return $a;
}

sub fromasc($)
{ my $a=$_[0];
  my $r; my $i;
  for($i=0; $i<length($a); $i=$i+2)
  { $r=$r.chr(hex(substr($a,$i,2)));
  } return $r;
}
