#!/usr/bin/perl
print "Content-type: text/html\n\n"; 
#!/usr/bin/perl
my @EWlocked;
use IO::Socket;
my @fltype;
my $EWB_tabpath="./";
my $EWB_template="<html><body bgcolor=lightyellow><div align=center><></body></html>";
my $EWB_format="12:5:h";
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
      {
        $parvalue[$i]=((split("\n", $p))[3]);
	$parvalue[$i]=substr($parvalue[$i], 0, length($parvalue[$i])-1);
	$fltype[$i]="text";
        if (substr($parvalue[$i],0, length("HidDenFieldDaTa")) eq
                  ("HidDenFieldDaTa"))
        { $parvalue[$i]=fromasc(substr($parvalue[$i],length("HidDenFieldDaTa")));
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
  { $res=$res."<input name=\"$n\" type=checkbox value=\"$v\">"; }
  if ($t eq "info")
  { $res="<table border=0><tr><td><b> $v </b>"; }
  if ($t eq "textarea")
  { $res=$res."<textarea name=\"$n\" cols=".$f[0]." rows=".$f[1].">$v</textarea>"; }
  #tipi composti
  if ($t eq "option") { $res=$res."<select name=\"$n\">"; }
  my @v=split(/;/,$v); my $p;
  my $sep; if ($f[2] eq "v") { $sep="<br>"; } else{ $sep=" &nbsp; "; };
  foreach $p(@v)
  {
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
    if ($t eq "option")
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
  if ($t eq "option") { $res=$res."</select>"; }
  if ($t ne "hidden") { $res=$res."</td></tr></table>"; }
  return $res."\n";
}

#liste - conversione da e a ascii
sub EWdate()
{ my @ti=localtime(time);
  return $ti[3]." ".($ti[4]+1)." ".($ti[5]+1900);
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
	my $TB_messaggi="messaggi:autore:titolo:messaggio:data";
	my $EWB_autore=""; 
	my $EWB_titolo=""; 
	my $EWB_messaggio=""; 
	my $EWB_data=""; 
	
my $EWB_v;

my $EWB_b;
$ax="20:8:h";
	$EWB_format=$ax;
	$ax="forum.html";
	$ax=EWload($ax);
	$EWB_template=$ax;
	
area100:;
	$ax=1;
	if (! ($ax)) {goto area101;};
	$ax="Inserisci";
	$EWB_b=$ax;
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$ax=CreateForm($ax,$bx,$cx,$dx);$EWB_v=$ax;
	$ax="messaggi:autore:titolo:messaggio:data";
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
area102:;
  if ($g > $#f) { goto area104; };
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
	$ax=$EWB_v;
	push @as,$ax;
	$ax="<b>";
	push @as,$ax;
	$ax=$EWB_autore;
	push @as,$ax;
	$ax=": ";
	push @as,$ax;
	$ax=$EWB_titolo;
	push @as,$ax;
	$ax=", ";
	push @as,$ax;
	$ax=$EWB_data;
	push @as,$ax;
	$ax="</b><br>";
	push @as,$ax;
	$ax=$EWB_messaggio;
	push @as,$ax;
	$ax="<br><br>";
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
	$EWB_v=$ax;
	
area103:;
  $g=$g+1;
  goto area102;
area104: ;
	
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
  { my $form=$EWB_v."";
$EWB_v=""; 
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     };
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     };
    if (index($form, "name=\"tabpath\"") <0)
     { $form=$form.CreateForm("tabpath", $EWB_tabpath, "hidden");
     };
    if (index($form, "name=\"autore\"") <0)
     { $form=$form.CreateForm("autore", $EWB_autore, "hidden");
     };
    if (index($form, "name=\"titolo\"") <0)
     { $form=$form.CreateForm("titolo", $EWB_titolo, "hidden");
     };
    if (index($form, "name=\"messaggio\"") <0)
     { $form=$form.CreateForm("messaggio", $EWB_messaggio, "hidden");
     };
    if (index($form, "name=\"data\"") <0)
     { $form=$form.CreateForm("data", $EWB_data, "hidden");
     };
    if (index($form, "name=\"v\"") <0)
     { $form=$form.CreateForm("v", $EWB_v, "hidden");
     };
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     };
$form=$form.CreateForm("EWjump", 0+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.369034444986941\">\
";
$form="<form name=\"ewb\" action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template))."\
\
"; exit(0); 
}

goto retlab1;
lab1:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_autore=get_par("autore"); 
$EWB_titolo=get_par("titolo"); 
$EWB_messaggio=get_par("messaggio"); 
$EWB_data=get_par("data"); 
$EWB_v=get_par("v"); 
$EWB_b=get_par("b"); 
retlab1:;
$ax=$EWB_b;
	push @as,$ax;
	$ax="Inserisci";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area105; };
	my $form="";
	push @as,"autore";
	push @as,$EWB_autore;
	push @as,"text";
	push @as,"autore";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"titolo";
	push @as,$EWB_titolo;
	push @as,"text";
	push @as,"titolo";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"messaggio";
	push @as,$EWB_messaggio;
	push @as,"textarea";
	push @as,"messaggio";
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
    if (index($form, "name=\"autore\"") <0)
     { $form=$form.CreateForm("autore", $EWB_autore, "hidden");
     } 
    if (index($form, "name=\"titolo\"") <0)
     { $form=$form.CreateForm("titolo", $EWB_titolo, "hidden");
     } 
    if (index($form, "name=\"messaggio\"") <0)
     { $form=$form.CreateForm("messaggio", $EWB_messaggio, "hidden");
     } 
    if (index($form, "name=\"data\"") <0)
     { $form=$form.CreateForm("data", $EWB_data, "hidden");
     } 
    if (index($form, "name=\"v\"") <0)
     { $form=$form.CreateForm("v", $EWB_v, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
$form=$form.CreateForm("EWjump", 1+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.409132394331635\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab2;
lab2:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_autore=get_par("autore"); 
$EWB_titolo=get_par("titolo"); 
$EWB_messaggio=get_par("messaggio"); 
$EWB_data=get_par("data"); 
$EWB_v=get_par("v"); 
$EWB_b=get_par("b"); 
retlab2:;
$ax=(EWdate ());
	$EWB_data=$ax;
	$ax=$EWB_autore;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	push @as,$ax;
	$ax=$EWB_titolo;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if (!($ax)) { goto area106; };
	$ax="messaggi:autore:titolo:messaggio:data";
$ax=ewb_add($ax);
	goto areaend106;
	
area106:;
	$ax="Errore<br>Autore e titolo non indicati.";
	$ax=print join(($ax), split("<>",$EWB_template));
	
areaend106:;
	goto areaend105;
	
area105:;
	;

areaend105:;
	goto area100;
	
area101:;
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
if ($lab==1) { goto lab1; }
if ($lab==2) { goto lab2; }
};

#Versione 4.11 - Gold
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
