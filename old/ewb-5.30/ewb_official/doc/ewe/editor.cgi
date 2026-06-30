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

sub checkpath()
{ my $a=$EWB_tabpath;
  if (substr($a,length($a)-1,1) ne "/")
  { $a=$a."/";
  }; return $a;
};

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

sub EWloaddir($)
{ my $a=""; my $el;
  my @f;
  if (opendir(dirr, $_[0]))
  { @f=readdir(dirr);
    foreach $el(@f)
    { $a=$a.toasc($el)." "; };
  }
  return $a;
}

sub EWsave($$)
{ my ($a, $b)=@_;
  my $r;
  $r = open oof, "> $a";
  if (!$r) { print "File di save non scrivibile. Controllare i permessi"; exit(0); };
  print oof $b;
  close oof;
  return $r;
}
#2.10 Inizio

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

sub CreateForm
{ #nomecampo, valore, tipo campo, nome a video
  my ($n,$v,$t,$vidname)=@_;
  if ($t eq "hidden")
  { #conversione di un campo hidden, analoga a push
    $v="HidDenFieldDaTa".toasc($v);
  }
  else
  { if (($t ne "textarea")&&($t ne "info"))
    { $v=join("%22", (split "\"", $v));
      $v=join("%09", (split "\t", $v));
      $v=join("%0A", (split "\n", $v));
      $v=join("%3C", (split "<", $v));
    };
  };
  my @f=split(":", $EWB_format);
  if ($f[0]<1) {$f[0]=12; };
  if ($f[1]<1) {$f[1]=5; };

  my $res;
  my $vn=$vidname;
  if (length($vidname) <2) { $vidname=$n; };
  $res="<table border=0><tr><td>$vidname</td><td>";

  if (($t eq "submit") && (length($vn)<2)) { $res="<table border=0><tr><td></td><td>"; };
  if ($t eq "file")
  { $res=$res."<input name=\"$n\" value=\"\" type=file>"; }
  if ($t eq "text")
  { $res=$res."<input name=\"$n\" value=\"$v\" size=".$f[0].">"; }
  if ($t eq "password")
  { $res=$res."<input name=\"$n\" type=password value=\"$v\" size=".$f[0].">"; }
  if ($t eq "hidden")
  { $res="<input name=\"$n\" type=hidden value=\"$v\">"; }
  if ($t eq "checkbox")
  { $res=$res."<input name=\"$n\" type=checkbox value=\"1\">"; }
  if ($t eq "info")
  { $res="<table border=0><tr><td><b> $v </b>"; }
  if ($t eq "textarea")
  { $res=$res."<textarea name=\"$n\" cols=".$f[0]." rows=".$f[1].">$v</textarea>"; }
  #tipi composti
  if ($t eq "option") { $res=$res."\n<select name=\"$n\">"; }
  if ($t eq "options") { $res=$res."\n<select name=\"$n\" multiple>"; }
  my @v=split(/;/,$v); my $p;

  if ($t eq "checks")
  { $res=$res."<input name=\"$n\" type=hidden value=\"\">\n";
  }

  my $sep; if ($f[2] eq "v") { $sep="<br>"; } else{ $sep=" &nbsp; "; };
  my $counter=0;
  foreach $p(@v)
  {
    if ($t eq "checks")
    { my $CV="";
      if (substr($p, 0, 1) eq  "*")
      { $p=substr($p, 1);
        $CV="checked";
      };
      $res=$res."<input name=\"EWcHECK_".$counter.$n."\" type=checkbox $CV value=\"$p\">$p $sep\n";
      $counter++;
    }
    if ($t eq "links")
    {  my ($vid,$dest)=split("->", $p);
       if ($dest eq "") { $dest=$vid; };
       $res=$res."<a href=\"$dest\">$vid</a>$sep"; }
    if ($t eq "radio")
    { if ( substr($p,0,1) eq "*")
      { $p=substr($p,1);
        $res=$res."<input type=radio name=\"$n\" value=\"$p\" checked>$p$sep";
      }
      else
      { $res=$res."<input type=radio name=\"$n\" value=\"$p\">$p$sep";
      };
    }
    if (($t eq "option") || ($t eq "options"))
    { if ( substr($p,0,1) eq "*")
      { $p=substr($p,1);
        $res=$res."<option value=\"$p\" selected>$p";
      }
      else
      { $res=$res."<option value=\"$p\">$p";
      };
     }
    if ($t eq "submit")
    { $res=$res."<input name=\"$n\" type=submit value=\"$p\">$sep"; }
  }
  if (($t eq "option")||($t eq "options")) { $res=$res."</select>"; }
  if ($t ne "hidden") { $res=$res."</td></tr></table>"; }
  return $res."\n";
}

#liste - conversione da e a ascii
sub stpop($)
{  my @a=split(/ /,$_[0]);
   $res=$a[$#a];
   $_[0]=substr($_[0],0,length($_[0])-(length($res)+1));
   return fromasc($res);
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

sub EWjoin($$)
{ my @a;
  foreach $p(split(/ /,$_[1]))
  { @a[$#a+1]=fromasc($p); }
  return join($_[0],@a);
}
##socket

sub EWdate()
{ my @ti=localtime(time);
  return $ti[3]." ".($ti[4]+1)." ".($ti[5]+1900);
}

sub EWtime()
{ my @ti=localtime(time);
  if ($ti[2] < 10)
  { $ti[2] = '0'.$ti[2];
  };
  if ($ti[1] < 10)
  { $ti[1] = '0'.$ti[1];
  };
  if ($ti[0] < 10)
  { $ti[0] = '0'.$ti[0];
  };
  return $ti[2].":".($ti[1]).":".($ti[0]);
}

#######################codice#utente#######################
#<EWB_usercode>
###########nuove funzioni della versione 2.0 #############

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
	$ax="&";
	push @as,$ax;
	$ax=(EWgetenvvar("QUERY_STRING"));
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$EWB_pars=$ax;
my $EWB_cmd;
	
my $EWB_b;
	
my $EWB_inf;
	
my $EWB_lod;
	$ax="";
	$EWB_lod=$ax;$ax=$EWB_pars;
	push @as,$ax;
	$ax=0;
	$bx=pop @as;$ax=getelem($bx,$ax);
	$EWB_cmd=$ax;$ax=$EWB_pars;
	push @as,$ax;
	$ax=1;
	$bx=pop @as;$ax=getelem($bx,$ax);
	$EWB_user=$ax;$ax=$EWB_pars;
	push @as,$ax;
	$ax=2;
	$bx=pop @as;$ax=getelem($bx,$ax);
	$EWB_pass=$ax;$ax=$EWB_pars;
	push @as,$ax;
	$ax=3;
	$bx=pop @as;$ax=getelem($bx,$ax);
	$EWB_lod=$ax;$ax="utenti:user:pass";
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
	$ax="mkdir files/";
	push @as,$ax;
	$ax=$EWB_user;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=qx{$ax};
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="nuovo";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area101; };
	$ax="nuovo programma amethyst<br>
  </b>Scegli un nome per il file ewb o html<b>";
	$EWB_inf=$ax;$ax="noname.ewb";
	$EWB_filename=$ax;$ax="Ok";
	$EWB_b=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="  ";
	push @as,"filename";
	push @as,$EWB_filename;
	push @as,"text";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
$form=$form.CreateForm("EWjump", 0+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.0813043926182253\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab1;
lab1:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
retlab1:;
$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx < $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx < $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if (!($ax)) { goto area102; };
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_filename=$ax;goto areaend102;
	
area102:;
	;

areaend102:;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	if (!($ax)) { goto area103; };
	$ax="<b>Errore</b><br>Il nome file ";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="  contiene caratteri non ammessi.";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend103;
	
area103:;
	;

areaend103:;
	
my $EWB_a;
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=EWloaddir($ax);
	$EWB_a=$ax;
area104:;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	if (! ($ax)) {goto area105;};
	$ax=stpop($EWB_a);
	push @as,$ax;
	$ax=$EWB_filename;
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area106; };
	$ax="<b>Errore<br></b>Il file gia' esiste.";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend106;
	
area106:;
	;

areaend106:;
	goto area104;
	
area105:;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	if (!($ax)) { goto area107; };
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_filename;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	push @as,$ax;
	$ax="'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' ";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="
'                                           Amethist web editor
'                                 Versione 0.2  By Enrico Betti
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=EWsave($bx,$ax);
	goto areaend107;
	
area107:;
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_filename;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	push @as,$ax;
	$ax="<!--
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' ";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="
'                                           Amethist web editor
'                                 Versione 0.2  By Enrico Betti
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
-->
<html>
  <head>
  </head>
  <body>

  <>

  </body>
</html>

";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=EWsave($bx,$ax);
	
areaend107:;
	$ax="load";
	$EWB_cmd=$ax;$ax=$EWB_filename;
	$EWB_lod=$ax;goto areaend101;
	
area101:;
	;

areaend101:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="view";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area108; };
	
my $EWB_a;
	
my $EWB_b;
	$ax=$EWB_user;
	$EWB_a=$ax;$ax=$EWB_lod;
	$EWB_b=$ax;$ax="files:user:filename";
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
  open rof,"> ".checkpath().".TB$n.bkp";
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
area109:;
  if ($g > $#f) { goto area111; };
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
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=$EWB_b;
	$bx=pop @as;
	$ax=($bx eq $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if ($ax) { goto area110;};
	print rof $l."\n";

area110:;
  $g=$g+1;
  goto area109;
area111: ;
	
  close rof;  
  #copia di un file sull altro
  open rif,checkpath().".TB$n.bkp";
  open rof, ">".checkpath().".TB$n.tab";
  my @fl=<rif>;
  my $rw;
  foreach $rw(@fl)
  { print rof $rw; };  
  close rif;
  close rof;
  tab_unlock($n);
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
  $ax="load";
	$EWB_cmd=$ax;$ax=$EWB_a;
	$EWB_user=$ax;$ax=$EWB_b;
	$EWB_lod=$ax;goto areaend108;
	
area108:;
	;

areaend108:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="load";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area112; };
	$ax=$EWB_lod;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area113; };
	
my $EWB_a;
	
my $EWB_k;
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=EWloaddir($ax);
	$EWB_a=$ax;$ax=$EWB_user;
	$EWB_k=$ax;
area114:;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	if (! ($ax)) {goto area115;};
	
my $EWB_b;
	$ax=stpop($EWB_a);
	$EWB_b=$ax;$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	$bx=pop @as;
	$ax=$bx || $ax;
	if (!($ax)) { goto area116; };
	
my $EWB_fnd;
	$ax="";
	$EWB_fnd=$ax;$ax="files:user:filename";
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
area117:;
  if ($g > $#f) { goto area119; };
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
	$ax=$EWB_k;
	$bx=pop @as;
	$ax=($bx eq $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=$EWB_b;
	$bx=pop @as;
	$ax=($bx eq $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if (!($ax)) { goto area118;};
	$ax=1;
	$EWB_fnd=$ax;
area118:;
  $g=$g+1;
  goto area117;
area119: ;
	
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
  $ax=$EWB_fnd;
	$ax=!$ax;
	if (!($ax)) { goto area120; };
	$ax=$EWB_lod;
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax=";";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_lod=$ax;goto areaend120;
	
area120:;
	;

areaend120:;
	goto areaend116;
	
area116:;
	;

areaend116:;
	goto area114;
	
area115:;
	$ax=$EWB_k;
	$EWB_user=$ax;$ax="Ok";
	$EWB_b=$ax;$ax="Load di file</b><br>Seleziona il file dall'elenco seguente<b>";
	$EWB_inf=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="  ";
	push @as,"lod";
	push @as,$EWB_lod;
	push @as,"option";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
    if (index($form, "name=\"k\"") <0)
     { $form=$form.CreateForm("k", $EWB_k, "hidden");
     } 
$form=$form.CreateForm("EWjump", 1+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.898595188834832\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab2;
lab2:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_a=get_par("a"); 
$EWB_k=get_par("k"); 
retlab2:;
goto areaend113;
	
area113:;
	;

areaend113:;
	
my $EWB_f;
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_lod;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=EWload($ax);
	$EWB_f=$ax;$ax=$EWB_f;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	if (!($ax)) { goto area121; };
	$ax=$EWB_lod;
	$EWB_filename=$ax;$ax="files:user:filename";
$ax=ewb_add($ax);
	$ax="85:22:h";
	$EWB_format=$ax;
area122:;
	$ax=1;
	if (! ($ax)) {goto area123;};
	$ax="Salva";
	$EWB_b=$ax;$ax="
<font size=-1 color=red>
Attenzione - premete salva prima di cambiare finestra per non perdere le modifiche</font>";
	$EWB_inf=$ax;my $form="";
	$ax="  ";
	push @as,"f";
	push @as,$EWB_f;
	push @as,"textarea";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"f\"") <0)
     { $form=$form.CreateForm("f", $EWB_f, "hidden");
     } 
$form=$form.CreateForm("EWjump", 2+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.748771916181919\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab3;
lab3:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_f=get_par("f"); 
retlab3:;
$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$EWB_f=$ax;$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_lod;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsave($bx,$ax);
	$ax="ok";
	$EWB_b=$ax;$ax="File salvato";
	$EWB_inf=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"f\"") <0)
     { $form=$form.CreateForm("f", $EWB_f, "hidden");
     } 
$form=$form.CreateForm("EWjump", 3+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.275072510677042\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab4;
lab4:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_f=get_par("f"); 
retlab4:;
goto area122;
	
area123:;
	goto areaend121;
	
area121:;
	$ax="<b>Errore</b><br>Il file non sembra esistere.";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	
areaend121:;
	exit (0);
	goto areaend112;
	
area112:;
	;

areaend112:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="close";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area124; };
	$ax=$EWB_lod;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area125; };
	
my $EWB_a;
	$ax=$EWB_user;
	$EWB_a=$ax;$ax="files:user:filename";
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
area126:;
  if ($g > $#f) { goto area128; };
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
	if (!($ax)) { goto area127;};
	$ax=$EWB_lod;
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=";";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_lod=$ax;
area127:;
  $g=$g+1;
  goto area126;
area128: ;
	
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
  $ax="Ok";
	$EWB_b=$ax;$ax="Chiusura di file</b><br>Seleziona il file dall'elenco
seguente<b>";
	$EWB_inf=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="  ";
	push @as,"lod";
	push @as,$EWB_lod;
	push @as,"option";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
$form=$form.CreateForm("EWjump", 4+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.929564301052853\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab5;
lab5:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_a=get_par("a"); 
retlab5:;
goto areaend125;
	
area125:;
	;

areaend125:;
	
my $EWB_a;
	
my $EWB_b;
	$ax=$EWB_user;
	$EWB_a=$ax;$ax=$EWB_filename;
	$EWB_b=$ax;$ax="files:user:filename";
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
  open rof,"> ".checkpath().".TB$n.bkp";
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
area129:;
  if ($g > $#f) { goto area131; };
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
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=$EWB_b;
	$bx=pop @as;
	$ax=($bx eq $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if ($ax) { goto area130;};
	print rof $l."\n";

area130:;
  $g=$g+1;
  goto area129;
area131: ;
	
  close rof;  
  #copia di un file sull altro
  open rif,checkpath().".TB$n.bkp";
  open rof, ">".checkpath().".TB$n.tab";
  my @fl=<rif>;
  my $rw;
  foreach $rw(@fl)
  { print rof $rw; };  
  close rif;
  close rof;
  tab_unlock($n);
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
  $ax="File ";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax=" chiuso";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend124;
	
area124:;
	;

areaend124:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="closeall";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area132; };
	
my $EWB_a;
	$ax=$EWB_user;
	$EWB_a=$ax;$ax="files:user:filename";
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
  open rof,"> ".checkpath().".TB$n.bkp";
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
area133:;
  if ($g > $#f) { goto area135; };
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
	if ($ax) { goto area134;};
	print rof $l."\n";

area134:;
  $g=$g+1;
  goto area133;
area135: ;
	
  close rof;  
  #copia di un file sull altro
  open rif,checkpath().".TB$n.bkp";
  open rof, ">".checkpath().".TB$n.tab";
  my @fl=<rif>;
  my $rw;
  foreach $rw(@fl)
  { print rof $rw; };  
  close rif;
  close rof;
  tab_unlock($n);
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
  $ax="Tutti i files sono stati chiusi";
	$ax=print join(($ax), split("<>",$EWB_template));
	goto areaend132;
	
area132:;
	;

areaend132:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="compila";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area136; };
	$ax=$EWB_lod;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area137; };
	
my $EWB_a;
	$ax=$EWB_user;
	$EWB_a=$ax;$ax="files:user:filename";
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
area138:;
  if ($g > $#f) { goto area140; };
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
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if (!($ax)) { goto area139;};
	$ax=$EWB_lod;
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=";";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_lod=$ax;
area139:;
  $g=$g+1;
  goto area138;
area140: ;
	
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
  $ax="Ok";
	$EWB_b=$ax;$ax="Compila file</b><br>Seleziona il file dall'elenco 
seguente<b>";
	$EWB_inf=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="  ";
	push @as,"lod";
	push @as,$EWB_lod;
	push @as,"option";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
$form=$form.CreateForm("EWjump", 5+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.720948467249386\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab6;
lab6:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_a=get_par("a"); 
retlab6:;
$ax=$EWB_a;
	$EWB_user=$ax;goto areaend137;
	
area137:;
	;

areaend137:;
	
my $EWB_res;
	
my $EWB_fn;
	$ax=$EWB_lod;
	$EWB_filename=$ax;$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$EWB_fn=$ax;$ax="cd files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/; ewb ";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" 2> .errors;  cd ../..";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=qx{$ax};
	$EWB_res=$ax;$ax="cd files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/; cp ";
	push @as,$ax;
	$ax=$EWB_fn;
	push @as,$ax;
	$ax=".cgi ";
	push @as,$ax;
	$ax=$EWB_fn;
	push @as,$ax;
	$ax="cgi.txt; cd ../..";
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
	$EWB_fn=$ax;$ax=$EWB_fn;
	$ax=qx{$ax};
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/.errors";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=EWload($ax);
	push @as,$ax;
	$ax="\n\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_res=$ax;$ax=$EWB_res;
	push @as,$ax;
	$ax="\nTerminato alle ";
	push @as,$ax;
	$ax=(EWtime ());
	push @as,$ax;
	$ax=" , ";
	push @as,$ax;
	$ax=(EWdate ());
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
	$EWB_res=$ax;$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$EWB_res=$ax;$ax="Risultato di compilazione:";
	$EWB_inf=$ax;$ax="</b><div align=left>";
	push @as,$ax;
	$ax=$EWB_res;
	push @as,$ax;
	$ax="<b></div>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_res=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"res";
	push @as,$EWB_res;
	push @as,"info";
	push @as,"res";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"res\"") <0)
     { $form=$form.CreateForm("res", $EWB_res, "hidden");
     } 
    if (index($form, "name=\"fn\"") <0)
     { $form=$form.CreateForm("fn", $EWB_fn, "hidden");
     } 
$form=$form.CreateForm("EWjump", 6+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.801683506599456\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab7;
lab7:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_res=get_par("res"); 
$EWB_fn=get_par("fn"); 
retlab7:;
exit (0);
	goto areaend136;
	
area136:;
	;

areaend136:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="compilatutto";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area141; };
	
my $EWB_res;
	$ax=$EWB_res;
	push @as,$ax;
	$ax="\nIniziato alle ";
	push @as,$ax;
	$ax=(EWtime ());
	push @as,$ax;
	$ax=" , ";
	push @as,$ax;
	$ax=(EWdate ());
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
	$EWB_res=$ax;$ax=$EWB_lod;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area142; };
	
my $EWB_a;
	$ax=$EWB_user;
	$EWB_a=$ax;$ax="files:user:filename";
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
area143:;
  if ($g > $#f) { goto area145; };
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
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if (!($ax)) { goto area144;};
	$ax=$EWB_res;
	push @as,$ax;
	$ax="<b>File: ";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="</b>\n";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_res=$ax;$ax="cd files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/ ; ewb ";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" 2> .errors";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=qx{$ax};
	$ax=$EWB_res;
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/.errors";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=EWload($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_res=$ax;
area144:;
  $g=$g+1;
  goto area143;
area145: ;
	
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
  goto areaend142;
	
area142:;
	;

areaend142:;
	$ax=$EWB_res;
	push @as,$ax;
	$ax="\nTerminato alle ";
	push @as,$ax;
	$ax=(EWtime ());
	push @as,$ax;
	$ax=" , ";
	push @as,$ax;
	$ax=(EWdate ());
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
	$EWB_res=$ax;$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$EWB_res=$ax;$ax="Risultato di compilazione:";
	$EWB_inf=$ax;$ax="</b><div align=left>";
	push @as,$ax;
	$ax=$EWB_res;
	push @as,$ax;
	$ax="<b></div>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_res=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"res";
	push @as,$EWB_res;
	push @as,"info";
	push @as,"res";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"res\"") <0)
     { $form=$form.CreateForm("res", $EWB_res, "hidden");
     } 
$form=$form.CreateForm("EWjump", 7+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.748926213052151\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab8;
lab8:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_res=get_par("res"); 
retlab8:;
exit (0);
	goto areaend141;
	
area141:;
	;

areaend141:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="esegui";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area146; };
	
my $EWB_res;
	$ax="";
	$EWB_res=$ax;$ax="12;5:v";
	$EWB_format=$ax;$ax=$EWB_lod;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area147; };
	
my $EWB_a;
	$ax=$EWB_user;
	$EWB_a=$ax;$ax="files:user:filename";
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
area148:;
  if ($g > $#f) { goto area150; };
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
	if (!($ax)) { goto area149;};
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	if (!($ax)) { goto area151; };
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$EWB_filename=$ax;$ax=$EWB_res;
	push @as,$ax;
	$ax="<br><br><a href=files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".cgi>
         ";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="</a>";
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
	$EWB_res=$ax;goto areaend151;
	
area151:;
	;

areaend151:;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	if (!($ax)) { goto area152; };
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$EWB_filename=$ax;$ax=$EWB_res;
	push @as,$ax;
	$ax="<br><br><a href=files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html>
         ";
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" (html)</a>";
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
	$EWB_res=$ax;goto areaend152;
	
area152:;
	;

areaend152:;
	
area149:;
  $g=$g+1;
  goto area148;
area150: ;
	
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
  goto areaend147;
	
area147:;
	;

areaend147:;
	$ax=$EWB_res;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend146;
	
area146:;
	;

areaend146:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="upload";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area153; };
	
my $EWB_a;
	
my $EWB_b;
	$ax="jpg;png;gif;bmp;html;htm";
	$EWB_a=$ax;my $form="";
	$ax="Seleziona il file";
	push @as,"filename";
	push @as,$EWB_filename;
	push @as,"file";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="Indica il nome del file";
	push @as,"b";
	push @as,$EWB_b;
	push @as,"text";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="Indica il tipo di file";
	push @as,"a";
	push @as,$EWB_a;
	push @as,"submit";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
$form=$form.CreateForm("EWjump", 8+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.0326591033866954\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab9;
lab9:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_a=get_par("a"); 
$EWB_b=get_par("b"); 
retlab9:;
$ax=$EWB_a;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	if (!($ax)) { goto area154; };
	$ax="Operazione annullata";
	$ax=print join(($ax), split("<>",$EWB_template));
	goto areaend154;
	
area154:;
	;

areaend154:;
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_b;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	push @as,$ax;
	$ax=$EWB_filename;
	$bx=pop @as;
	$ax=EWsave($bx,$ax);
	$ax="File portato sul server.";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend153;
	
area153:;
	;

areaend153:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="files";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area155; };
	
my $EWB_a;
	
my $EWB_b;
	
my $EWB_res;
	$ax="<br>";
	$EWB_res=$ax;$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=EWloaddir($ax);
	$EWB_a=$ax;
area156:;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	if (! ($ax)) {goto area157;};
	$ax=stpop($EWB_a);
	$EWB_b=$ax;$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	if (!($ax)) { goto area158; };
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx < $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx < $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx < $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx < $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	$bx=pop @as;
	$ax=$bx && $ax;
	$bx=pop @as;
	$ax=$bx && $ax;
	if (!($ax)) { goto area159; };
	$ax=$EWB_res;
	push @as,$ax;
	$ax="<img src=files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax=" width=50>";
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
	$EWB_res=$ax;goto areaend159;
	
area159:;
	;

areaend159:;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx < $ax);
	if (!($ax)) { goto area160; };
	$ax=$EWB_res;
	push @as,$ax;
	$ax="<a href=\"files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="\">";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="</a><br>";
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
	$EWB_res=$ax;goto areaend160;
	
area160:;
	$ax=$EWB_res;
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="<br>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_res=$ax;
areaend160:;
	goto areaend158;
	
area158:;
	;

areaend158:;
	goto area156;
	
area157:;
	$ax="<b>Elenco dei files su server</b><br>";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend155;
	
area155:;
	;

areaend155:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="del";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area161; };
	
my $EWB_a;
	
my $EWB_b;
	
my $EWB_res;
	$ax="";
	$EWB_res=$ax;$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=EWloaddir($ax);
	$EWB_a=$ax;
area162:;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	if (! ($ax)) {goto area163;};
	$ax=stpop($EWB_a);
	$EWB_b=$ax;$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx >= $ax);
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	$bx=pop @as;
	$ax=$bx || $ax;
	if (!($ax)) { goto area164; };
	$ax=$EWB_res;
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax=";";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_res=$ax;goto areaend164;
	
area164:;
	;

areaend164:;
	goto area162;
	
area163:;
	$ax="Cancellazione file</b><br>Seleziona i files da cancellare. Puoi selezionare piu'files tenendo premuto 
ctrl.<b>";
	$EWB_inf=$ax;$ax="Ok";
	$EWB_b=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="  ";
	push @as,"res";
	push @as,$EWB_res;
	push @as,"options";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"res\"") <0)
     { $form=$form.CreateForm("res", $EWB_res, "hidden");
     } 
$form=$form.CreateForm("EWjump", 9+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.91089432576041\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab10;
lab10:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_a=get_par("a"); 
$EWB_b=get_par("b"); 
$EWB_res=get_par("res"); 
retlab10:;
$ax=";";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$EWB_a=$ax;
area165:;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	if (! ($ax)) {goto area166;};
	$ax=stpop($EWB_a);
	$EWB_b=$ax;
my $EWB_k;
	$ax="rm -f files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_b;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=qx{$ax};
	$ax=$EWB_user;
	$EWB_k=$ax;$ax="files:user:filename";
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
  open rof,"> ".checkpath().".TB$n.bkp";
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
area167:;
  if ($g > $#f) { goto area169; };
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
	$ax=$EWB_k;
	$bx=pop @as;
	$ax=($bx eq $ax);
	push @as,$ax;
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=$EWB_b;
	$bx=pop @as;
	$ax=($bx eq $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if ($ax) { goto area168;};
	print rof $l."\n";

area168:;
  $g=$g+1;
  goto area167;
area169: ;
	
  close rof;  
  #copia di un file sull altro
  open rif,checkpath().".TB$n.bkp";
  open rof, ">".checkpath().".TB$n.tab";
  my @fl=<rif>;
  my $rw;
  foreach $rw(@fl)
  { print rof $rw; };  
  close rif;
  close rof;
  tab_unlock($n);
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
  $ax=$EWB_k;
	$EWB_user=$ax;goto area165;
	
area166:;
	$ax="<b>cancellazione files</b><br>files (";
	push @as,$ax;
	$ax=$EWB_res;
	push @as,$ax;
	$ax=") cancellati.\n";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend161;
	
area161:;
	;

areaend161:;
	$ax=$EWB_cmd;
	push @as,$ax;
	$ax="tbedit";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area170; };
	$ax=".TB";
	push @as,$ax;
	$ax=$EWB_lod;
	push @as,$ax;
	$ax=".tab";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_lod=$ax;
my $EWB_f;
	$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_lod;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=EWload($ax);
	$EWB_f=$ax;
my $EWB_a;
	
my $EWB_b;
	
my $EWB_leng;
	$ax=85;
	$EWB_leng=$ax;$ax="\n";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$EWB_a=$ax;
area171:;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	if (! ($ax)) {goto area172;};
	$ax=stpop($EWB_a);
	$EWB_b=$ax;$ax=$EWB_b;
	$ax=length($ax);
	push @as,$ax;
	$ax=$EWB_leng;
	$bx=pop @as;
	$ax=($bx > $ax);
	if (!($ax)) { goto area173; };
	$ax=$EWB_b;
	$ax=length($ax);
	$EWB_leng=$ax;goto areaend173;
	
area173:;
	;

areaend173:;
	goto area171;
	
area172:;
	$ax=$EWB_leng;
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx + $ax;
	push @as,$ax;
	$ax=":22:h";
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_format=$ax;
area174:;
	$ax=1;
	if (! ($ax)) {goto area175;};
	$ax="Salva";
	$EWB_b=$ax;$ax="
<font size=-1 color=red>
Attenzione - premete salva prima di cambiare finestra per non perdere le modifiche</font>";
	$EWB_inf=$ax;my $form="";
	$ax="  ";
	push @as,"f";
	push @as,$EWB_f;
	push @as,"textarea";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"f\"") <0)
     { $form=$form.CreateForm("f", $EWB_f, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"leng\"") <0)
     { $form=$form.CreateForm("leng", $EWB_leng, "hidden");
     } 
$form=$form.CreateForm("EWjump", 10+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.589226754165605\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab11;
lab11:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_f=get_par("f"); 
$EWB_a=get_par("a"); 
$EWB_b=get_par("b"); 
$EWB_leng=get_par("leng"); 
retlab11:;
$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=" ";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="..";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="<br>";
	push @as,$ax;
	$ax="\n";
	push @as,$ax;
	$ax=$EWB_res;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=4;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=".html";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_filename;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_filename;
	$ax=length($ax);
	push @as,$ax;
	$ax=5;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".txt";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".jpg";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".png";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".gif";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".bmp";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".ewb";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".cgi";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".htm";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax="";
	push @as,$ax;
	$ax="\r";
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsplit($bx,$ax);
	$bx=pop @as;
	$ax=EWjoin($bx,$ax);
	$EWB_f=$ax;$ax="files/";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="/";
	push @as,$ax;
	$ax=$EWB_lod;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	push @as,$ax;
	$ax=$EWB_f;
	$bx=pop @as;
	$ax=EWsave($bx,$ax);
	$ax="ok";
	$EWB_b=$ax;$ax="File salvato";
	$EWB_inf=$ax;my $form="";
	push @as,"inf";
	push @as,$EWB_inf;
	push @as,"info";
	push @as,"inf";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     } 
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     } 
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     } 
    if (index($form, "name=\"sqldbase\"") <0)
     { $form=$form.CreateForm("sqldbase", $EWB_sqldbase, "hidden");
     } 
    if (index($form, "name=\"sqlserver\"") <0)
     { $form=$form.CreateForm("sqlserver", $EWB_sqlserver, "hidden");
     } 
    if (index($form, "name=\"sqluser\"") <0)
     { $form=$form.CreateForm("sqluser", $EWB_sqluser, "hidden");
     } 
    if (index($form, "name=\"sqlpassword\"") <0)
     { $form=$form.CreateForm("sqlpassword", $EWB_sqlpassword, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"pass\"") <0)
     { $form=$form.CreateForm("pass", $EWB_pass, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"filename\"") <0)
     { $form=$form.CreateForm("filename", $EWB_filename, "hidden");
     } 
    if (index($form, "name=\"pars\"") <0)
     { $form=$form.CreateForm("pars", $EWB_pars, "hidden");
     } 
    if (index($form, "name=\"cmd\"") <0)
     { $form=$form.CreateForm("cmd", $EWB_cmd, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lod\"") <0)
     { $form=$form.CreateForm("lod", $EWB_lod, "hidden");
     } 
    if (index($form, "name=\"f\"") <0)
     { $form=$form.CreateForm("f", $EWB_f, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"leng\"") <0)
     { $form=$form.CreateForm("leng", $EWB_leng, "hidden");
     } 
$form=$form.CreateForm("EWjump", 11+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.611501016411673\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab12;
lab12:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sqldbase=get_par("sqldbase"); 
$EWB_sqlserver=get_par("sqlserver"); 
$EWB_sqluser=get_par("sqluser"); 
$EWB_sqlpassword=get_par("sqlpassword"); 
$EWB_user=get_par("user"); 
$EWB_pass=get_par("pass"); 
$EWB_user=get_par("user"); 
$EWB_filename=get_par("filename"); 
$EWB_pars=get_par("pars"); 
$EWB_cmd=get_par("cmd"); 
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lod=get_par("lod"); 
$EWB_f=get_par("f"); 
$EWB_a=get_par("a"); 
$EWB_b=get_par("b"); 
$EWB_leng=get_par("leng"); 
retlab12:;
goto area174;
	
area175:;
	exit (0);
	goto areaend170;
	
area170:;
	;

areaend170:;
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
if ($lab==1) { goto lab1; }
if ($lab==2) { goto lab2; }
if ($lab==3) { goto lab3; }
if ($lab==4) { goto lab4; }
if ($lab==5) { goto lab5; }
if ($lab==6) { goto lab6; }
if ($lab==7) { goto lab7; }
if ($lab==8) { goto lab8; }
if ($lab==9) { goto lab9; }
if ($lab==10) { goto lab10; }
if ($lab==11) { goto lab11; }
if ($lab==12) { goto lab12; }
};

#Amethist easyweb v. 5.23 - easy 
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

sub delchar($)
{ my $a=join("<EWB_ASTErISCO>",split "#", $_[0]);
  $a=join("<EWB_LineFEEd>", split(chr(0x0D), $a));
  return join("<EWB_CARRIaGErETURN>", split("\n", $a));
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
