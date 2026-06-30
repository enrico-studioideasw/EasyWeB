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
          $parvalue[$i]=substr($parvalue[$i], 0, (length($parvalue[$i])-2));
	}
        $i++;
      }
    }
  }
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
sub tab_wlock($)
{ my $name=$_[0];
  $name=((split(":", $name))[0]);
  my $i; my $d=0;
  if (!tab_islock($name))
  { while (($i<10)&&($d==0))
    { if (!(open (III, "< ".checkpath()."TB".$name.".lck")))
      { if (!(open (OOO, "> ".checkpath()."TB".$name.".lck")))
        { print  "Attenzione: non posso creare files.\n";
          exit(0);
        };
        close OOO;$d=1;
      }
      else
      { close III; sleep (1); $i++;
      }
    }
  }
  push @EWlocked, $name;
}

sub tab_unlock($)
{ my $name=$_[0];
  $name=((split(":", $name))[0]);
  my @a; my $l;
  my $count=0;
  foreach $l(@EWlocked)
  { if (($l ne $name) || ($count>0)) { push @a,$l; }
    if ($l eq $name) {++$count; };
  }
  @EWlocked=@a;
  if ($count=1) { unlink(checkpath()."TB".$name.".lck"); };
}

sub tab_read($)
{ #esegue la lettura di una tabella gia bloccata.
  my @t=split(":", $_[0]);
  open III,"< ".checkpath().".TB$t[0].tab";
    my $k=join("",<III>);
    my @r=split("\n",$k);
  close III;
  return @r;
};
sub addchar($)
{ my $a=join("#",split "<EWB_ASTErISCO>", $_[0]);
  $a=join(chr(0x0D),split("<EWB_LineFEEd>", $a));
  return join("\n", split("<EWB_CARRIaGErETURN>", $a));
}


my @parname;
my @parvalue;
srand(time());
read_form_buffer();
#jumper area
if (get_par("EWjump")>0)
{ jumper(get_par("EWjump"));
};
my $ax,$bx,$cx,$dx,@as;
	my $TB_prova="prova:a:b:c";
	my $EWB_a=""; 
	my $EWB_b=""; 
	my $EWB_c=""; 
	$ax="uno";
	$EWB_a=$ax;$ax="due";
	$EWB_b=$ax;$ax="tre";
	$EWB_c=$ax;$ax="prova:a:b:c";
$ax=ewb_add($ax);
	$ax="qua";
	$EWB_a=$ax;$ax="cin";
	$EWB_b=$ax;$ax="sei";
	$EWB_c=$ax;$ax="prova:a:b:c";
$ax=ewb_add($ax);
	$ax="prova:a:b:c";
  $tot=$ax;
  $n=((split(":",$tot))[0]);
  #modifica 030425P inizio
  $ax=join("\n",@p);
  push @as, $ax;
  #modifica 030425P fine
  @p=split(":",substr($tot,length($n)+1));
  tab_wlock($n);

  #modifica 030423 inizio
  #my @f=tab_read($tot);
  $ax=join("\n",@f);
  push @as, $ax;
  @f=tab_read($tot);
  #modifica 030423 fine

  tab_unlock($n);
  #modifica 030423 inizio
  #my $l; my $i=0;
  #my $g=0;
  $ax=join("\n",@aa);
  push @as, $ax;
  push @as, $ct;
  push @as, $i;
  push @as, $l; 
  push @as, $g;
  $i=0;
  $g=0;
  #modifica 030423 fine
  #modifica 030424 inizio
  push @as, $k;
  push @as, $found;
  #modifica 030424 fine
area100:;
  if ($g > $#f) { goto area102; };
  $l=$f[$g]; $l=substr($l,0,length($l));
  #modifica 030424 inizio
  #my $k;
  #my $found=0;
  $k=0;
  $found=0;
  #modifica 030424 fine
  $i++;
  for ($k=$i; $k<=$#f; $k++)
  { if ( ((split( "#",$l))[0]) eq ((split("#",$f[$k]))[0]) )
    { $found=1; };
  }
  if ($found) { goto area101;};
  @aa=split("#", $l);
  #my $ct=0;
  $ct=0;
  $code="";
  foreach $p(@p)
  { $code=$code."\$EWB_".$p."=addchar(\$aa[$ct]);
";
    $ct++;
  };
  eval($code);
	$ax=$EWB_a;
	push @as,$ax;
	$ax=" ";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax=" ";
	push @as,$ax;
	$ax=$EWB_c;
	push @as,$ax;
	$ax="\n";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	print $ax;
	
area101:;
  $g=$g+1;
  goto area100;
area102:;
	
  $found=pop @as;
  $k=pop @as;
  
  $g=pop @as;
  $l=pop @as; 
  $i=pop @as;
  $ct=pop @as;
  $ax=pop @as;
  @aa=split("\n", $ax);
  $ax=pop @as;
  @f=split("\n", $ax);
  #modifica 030425P inizio
  $ax=pop @as;
  @p=split("\n", $ax);
$ax="\n";
	print $ax;
	$ax="carciofo";
	$EWB_b=$ax;$ax="prova:a:b:c";
$bx=(split(":", $ax))[1];
	$ax=ewb_update($ax, eval("\$EWB_$bx"));
	$ax="prova:a:b:c";
  $tot=$ax;
  $n=((split(":",$tot))[0]);
  #modifica 030425P inizio
  $ax=join("\n",@p);
  push @as, $ax;
  #modifica 030425P fine
  @p=split(":",substr($tot,length($n)+1));
  tab_wlock($n);

  #modifica 030423 inizio
  #my @f=tab_read($tot);
  $ax=join("\n",@f);
  push @as, $ax;
  @f=tab_read($tot);
  #modifica 030423 fine

  tab_unlock($n);
  #modifica 030423 inizio
  #my $l; my $i=0;
  #my $g=0;
  $ax=join("\n",@aa);
  push @as, $ax;
  push @as, $ct;
  push @as, $i;
  push @as, $l; 
  push @as, $g;
  $i=0;
  $g=0;
  #modifica 030423 fine
  #modifica 030424 inizio
  push @as, $k;
  push @as, $found;
  #modifica 030424 fine
area103:;
  if ($g > $#f) { goto area105; };
  $l=$f[$g]; $l=substr($l,0,length($l));
  #modifica 030424 inizio
  #my $k;
  #my $found=0;
  $k=0;
  $found=0;
  #modifica 030424 fine
  $i++;
  for ($k=$i; $k<=$#f; $k++)
  { if ( ((split( "#",$l))[0]) eq ((split("#",$f[$k]))[0]) )
    { $found=1; };
  }
  if ($found) { goto area104;};
  @aa=split("#", $l);
  #my $ct=0;
  $ct=0;
  $code="";
  foreach $p(@p)
  { $code=$code."\$EWB_".$p."=addchar(\$aa[$ct]);
";
    $ct++;
  };
  eval($code);
	$ax=$EWB_a;
	push @as,$ax;
	$ax=" ";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax=" ";
	push @as,$ax;
	$ax=$EWB_c;
	push @as,$ax;
	$ax="\n";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	print $ax;
	
area104:;
  $g=$g+1;
  goto area103;
area105:;
	
  $found=pop @as;
  $k=pop @as;
  
  $g=pop @as;
  $l=pop @as; 
  $i=pop @as;
  $ct=pop @as;
  $ax=pop @as;
  @aa=split("\n", $ax);
  $ax=pop @as;
  @f=split("\n", $ax);
  #modifica 030425P inizio
  $ax=pop @as;
  @p=split("\n", $ax);
$ax="\n";
	print $ax;
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
};

#Amethist easyweb v. 5.10
#Enrico Betti e Paride Dominici



sub ewb_add($)
{  my $cond="my \$line=";
   my $tab=$_[0];
   my $count=0;
   my @tot=split(":",$tab);
   tab_wlock($tab);
   foreach $field(@tot)
   { if ($count>0)
     { $cond=$cond."delchar(\$EWB_".$field.")";
       if ($count<$#tot) {$cond=$cond.".\"#\"."; };
     }
     $count++;
  }
  $cond.=";
if (!(open oof,\">> ".checkpath().".TB$tot[0].tab\"))
{ print \"\\nAttenzione: non posso creare files\\n\";
  exit(0);
};
print oof \$line.\"\\n\";
close oof;
";
  eval($cond);
  tab_unlock($tab);
  return ($res ne "");
};

sub ewb_update($$)
{ my ($n1, $k)=@_;
  my ($n)=split(":", $n1);
  my $res=tab_del($n, $k);
  if ($res ne "")
  { 
    return ewb_add($n1);
  }
  else { return ""; };
}

sub checkpath()
{ my $a=$EWB_tabpath;
  if (substr($a,length($a)-1,1) ne "/")
  { $a=$a."/";
  }; return $a;
};

sub EWconv($)
{ my $a=$_[0];
  $a=join("%3C", split("%253C", $a));
  $a=~ tr/+/ /;
  $a=~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C",hex($1))/eg;
  $a=join("
", split("%0A", $a));
  $a=join("	", split("%09", $a));
  $a=join("\"", split("%22", $a));
  $a=join("<", split("%3C", $a));
  return $a;
}

sub tab_islock($)
{ my $name=$_[0];
  my $l; my $r=0;
  foreach $l(@EWlocked)
  { if ($l eq $name) {$r=1;}
  } return $r;
}

sub tab_del($$)
{ my ($n,$k)=@_; my $r=""; my $l;
  tab_wlock($n);
  open III,"< ".checkpath().".TB$n.tab";
  my @a=(<III>); close III;
  open oof,"> ".checkpath().".TB$n.tab";
  foreach $l(@a)
  {
    if (substr($l,length($l)-1,1) eq "\n")
    {  $l=substr($l, 0, length($l)-1);
    }
    if ( ((split("#",$l))[0]) eq delchar($k))
    { $r=1;}
    else
    { print oof $l."\n";
    }
  }
  close oof; tab_unlock($n); return $r;
}

sub delchar($)
{ my $a=join("<EWB_ASTErISCO>",split "#", $_[0]);
  $a=join("<EWB_LineFEEd>", split(chr(0x0D), $a));
  return join("<EWB_CARRIaGErETURN>", split("\n", $a));
}

sub fromasc($)
{ my $a=$_[0];
  my $r; my $i;
  for($i=0; $i<length($a); $i=$i+2)
  { $r=$r.chr(hex(substr($a,$i,2)));
  } return $r;
}
