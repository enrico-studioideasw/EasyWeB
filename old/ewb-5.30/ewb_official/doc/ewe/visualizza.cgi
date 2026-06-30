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

sub getelem($$)
{  my @a=split(/ /,$_[0]);
   $res=fromasc($a[$_[1]]);
   return $res;
}

sub EWsplit($$)
{ my $a=""; my $p;
  foreach $p(split( $_[0], $_[1]))
  { $a=$a.toasc($p)." ";
  } return $a;
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
	my $TB_utenti="utenti:user:pass";
	my $EWB_user=""; 
	my $EWB_pass=""; 
		my $TB_files="files:user:filename";
	my $EWB_user=""; 
	my $EWB_filename=""; 
	
my $EWB_pars;
	$ax="pages/left.html";
	$ax=EWload($ax);
	$EWB_template=$ax;$ax="&";
	push @as,$ax;
	$ax=(EWgetenvvar("QUERY_STRING"));
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$EWB_pars=$ax;
my $EWB_cmd;
	
my $EWB_b;
	
my $EWB_inf;
	$ax=$EWB_pars;
	push @as,$ax;
	$ax=0;
	$bx=pop @as;$ax=getelem($bx,$ax);
	$EWB_user=$ax;$ax=$EWB_pars;
	push @as,$ax;
	$ax=1;
	$bx=pop @as;$ax=getelem($bx,$ax);
	$EWB_pass=$ax;$ax="utenti:user:pass";
$ax=ewb_login($ax);
	$ax=!$ax;
	if (!($ax)) { goto area100; };
	$ax="Login non valido.  Contattare l'amministratore";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend100;
	
area100:;
	;

areaend100:;
	
my $EWB_a;
	$ax=$EWB_user;
	$EWB_a=$ax;
my $EWB_res;
	$ax="";
	$EWB_res=$ax;$ax="files:user:filename";
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
  #modifica 030424 inizio
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
  push @as, $k;
  #modifica 030424 fine
area101:;
  if ($g > $#f) { goto area103; };
  $l=$f[$g]; $l=substr($l,0,length($l));
  #modifica 030424 inizio
  #my $k;
  #push @as, $k;
  $k=0;
  #modifica 030424 fine
  $i++;
  
  #my @aa=split("#", $l);
  @aa=split("#", $l);
  $ct=0;
  $code="";
  foreach $p(@p)
  { $code=$code."\$EWB_".$p."=addchar(\$aa[$ct]);
";
    $ct++;
  };
  eval($code);
	$ax=$EWB_user;
	push @as,$ax;
	$ax=$EWB_a;
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area102;};
	
my $EWB_fn;
	$ax=$EWB_res;
	push @as,$ax;
	$ax="
<a href=editor.cgi?view&";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="&";
	push @as,$ax;
	$ax=$EWB_pass;
	push @as,$ax;
	$ax="&";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="&end target=right>
";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="</a><br><br>";
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
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_res=$ax;
area102:;
  $g=$g+1;
  goto area101;
area103: ;
	
  $k=pop @as;
  $g=pop @as;
  $l=pop @as; 
  $i=pop @as;
  $ct=pop @as;
  $ax=pop @as;
  @aa=split("\n", $ax);
  $ax=pop @as;
  @f=split("\n", $ax);
  $ax=pop @as;
  @p=split("\n", $ax);
  $ax=$EWB_res;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area104; };
	$ax="No file";
	$EWB_res=$ax;goto areaend104;
	
area104:;
	;

areaend104:;
	$ax=$EWB_res;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
};

#Amethist easyweb v. 5.21 - easy 
#Enrico Betti e Paride Dominici



sub ewb_login($)
{  $cond="\$res=join(\"\",((";
   $tab=$_[0];
   my $count=0;
   my @tot=split(":",$tab);
   foreach $field(@tot)
   { if ($count>0)
     { $cond=$cond."\$EWB_".$field;
       if ($count<$#tot) {$cond=$cond.","; };
     }
     $count++;
  }
  $cond=$cond.")=tab_login(\"$tot[0]\",\$EWB_";
  $cond=$cond.$tot[1].", \$EWB_".$tot[2].")));";
  eval($cond);
  return ($res ne "");
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
#  $a=~ tr/+/ /;
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

sub tab_login($$$)
{ my ($n,$k,$p)=@_;
  my @r; my @a=tab_exist($n,$k);
  if ($a[1] eq $p) { @r=@a; }
  return @r;
}

sub toasc($)
{ my $a=$_[0];
  my $r; my $i;
  for($i=0; $i<length($a); $i++)
  { my $k=ord(substr($a,$i,1));
    if ($k<16) {$r=$r."0";};
    $r=$r.sprintf("%x",$k);
  } return $r;
}

sub fromasc($)
{ my $a=$_[0];
  my $r; my $i;
  for($i=0; $i<length($a); $i=$i+2)
  { $r=$r.chr(hex(substr($a,$i,2)));
  } return $r;
}

sub tab_exist($$)
{ my ($n,$k)=@_;
  tab_wlock($n);
  open III,"< ".checkpath().".TB$n.tab";
  my $l; my @r;
  foreach $l(<III>)
  { #tolgo da $l il caporiga, o non funziona la exist.
    if (substr($l,length($l)-1,1) eq "\n")
    {  $l=substr($l, 0, length($l)-1);
    }
    if ( ((split("#",$l))[0]) eq delchar($k))
    { my $a;
      @r=();
      foreach $a(split("#",$l))
      { push @r, addchar($a);
  } } }
  close III; tab_unlock($n); return @r;
}

sub delchar($)
{ my $a=join("<EWB_ASTErISCO>",split "#", $_[0]);
  $a=join("<EWB_LineFEEd>", split(chr(0x0D), $a));
  return join("<EWB_CARRIaGErETURN>", split("\n", $a));
}
