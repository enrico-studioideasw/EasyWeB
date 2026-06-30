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

sub tab_sort($$)
{ my ($name,$s)=@_; my $r=""; my $l;
  my $n=((split(":",$name))[0]);
  $name=substr($name,length($n)+1);
  my $p; my $c=0;
  foreach $p(split(":",$name))
  { if ($p eq $s) { $k=$c; };
    $c++;
  }
  my @b; $c=0;
  tab_wlock($n);
  open III,"<".checkpath().".TB$n.tab";
  my @a=(<III>); close III;
  foreach $p(@a)
  { my $t;
    my @p=split("#",$p);
    $t=$p[0];
    $p[0]=$p[$k];
    $p[$k]=$t;
    $b[$c]=join("#",@p);
    $c=$c+1;
  }
  @b=sort(@b);
  $c=0;
  foreach $p(@b)
  { my $t;
    my @p=split("#",$p);
    $t=$p[0];
    $p[0]=$p[$k];
    $p[$k]=$t;
    $a[$c]=join("#",@p);
    $c=$c+1;
  }
  open oof,"> ".checkpath().".TB$n.tab";
  foreach $l(@a)
  { print oof $l;
  }
  close oof; tab_unlock($n); return $r;
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
	
my $EWB_inf;
	
my $EWB_b;
	
my $EWB_infb;
	$ax="Qui effettuate il login come amministratore (root)";
	$EWB_inf=$ax;$ax="<br><br></b><a href=start.cgi>Torna al servizio web editor</a><b>";
	$EWB_infb=$ax;$ax="ok";
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
	push @as,"user";
	push @as,$EWB_user;
	push @as,"text";
	push @as,"user";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"pass";
	push @as,$EWB_pass;
	push @as,"password";
	push @as,"pass";
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
	push @as,"infb";
	push @as,$EWB_infb;
	push @as,"info";
	push @as,"infb";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 0+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.629976448210986\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
retlab1:;
$ax=$EWB_user;
	push @as,$ax;
	$ax="root";
	$bx=pop @as;
	$ax=($bx eq $ax);
	push @as,$ax;
	$ax="utenti:user:pass";
$ax=ewb_login($ax);
	$bx=pop @as;
	$ax=$bx && $ax;
	if (!($ax)) { goto area100; };
	
area101:;
	$ax=1;
	if (! ($ax)) {goto area102;};
	$ax="12:5:v";
	$EWB_format=$ax;
my $EWB_pb;
	$ax="<br><br></b><a href=start.cgi>Torna al servizio web editor</a><b>";
	$EWB_inf=$ax;$ax="Crea utente;Elenca utenti;Cancella utente";
	$EWB_b=$ax;my $form="";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
    if (index($form, "name=\"pb\"") <0)
     { $form=$form.CreateForm("pb", $EWB_pb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 1+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.612932257211895\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
$EWB_pb=get_par("pb"); 
retlab2:;
$ax="";
	$EWB_user=$ax;$ax="";
	$EWB_pass=$ax;$ax="";
	$EWB_pb=$ax;$ax=$EWB_b;
	push @as,$ax;
	$ax="Elenca utenti";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area103; };
	$ax="Elenco utenti per il web editor</b><br><br>";
	$EWB_inf=$ax;$ax="utenti:user:pass";
push @as,$ax;
	$ax="user";
	$bx=pop @as;
	$ax=tab_sort($bx,$ax);$ax="utenti:user:pass";
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
area104:;
  if ($g > $#f) { goto area106; };
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
  if ($found) { goto area105;};
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
	$ax=$EWB_inf;
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="<br>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_inf=$ax;
area105:;
  $g=$g+1;
  goto area104;
area106:;
	
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
$ax="ok";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
    if (index($form, "name=\"pb\"") <0)
     { $form=$form.CreateForm("pb", $EWB_pb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 2+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.227866477138502\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
$EWB_pb=get_par("pb"); 
retlab3:;
goto areaend103;
	
area103:;
	;

areaend103:;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="Crea utente";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area107; };
	$ax="Crea utente o cambia password";
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
	push @as,"user";
	push @as,$EWB_user;
	push @as,"text";
	push @as,"user";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="password";
	push @as,"pass";
	push @as,$EWB_pass;
	push @as,"password";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="ripeti password";
	push @as,"pb";
	push @as,$EWB_pb;
	push @as,"password";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
    if (index($form, "name=\"pb\"") <0)
     { $form=$form.CreateForm("pb", $EWB_pb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 3+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.95259801679908\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
$EWB_pb=get_par("pb"); 
retlab4:;
$ax=$EWB_user;
	$ax=length($ax);
	push @as,$ax;
	$ax=2;
	$bx=pop @as;
	$ax=($bx < $ax);
	push @as,$ax;
	$ax=$EWB_pass;
	push @as,$ax;
	$ax=$EWB_pb;
	$bx=pop @as;
	$ax=($bx ne $ax);
	$bx=pop @as;
	$ax=$bx || $ax;
	if (!($ax)) { goto area108; };
	$ax="ok";
	$EWB_b=$ax;$ax="Utente non valido o le password non corrispondono";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
    if (index($form, "name=\"pb\"") <0)
     { $form=$form.CreateForm("pb", $EWB_pb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 4+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.113541906595021\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
$EWB_pb=get_par("pb"); 
retlab5:;
goto areaend108;
	
area108:;
	$ax="Utente ";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax=" inserito.";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_inf=$ax;$ax="ok";
	$EWB_b=$ax;$ax="utenti:user:pass";
$ax=ewb_add($ax);
	my $form="";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
    if (index($form, "name=\"pb\"") <0)
     { $form=$form.CreateForm("pb", $EWB_pb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 5+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.346441813730038\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
$EWB_pb=get_par("pb"); 
retlab6:;

areaend108:;
	goto areaend107;
	
area107:;
	;

areaend107:;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="Cancella utente";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area109; };
	$ax="Cancella utente";
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
	push @as,"user";
	push @as,$EWB_user;
	push @as,"text";
	push @as,"user";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
    if (index($form, "name=\"pb\"") <0)
     { $form=$form.CreateForm("pb", $EWB_pb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 6+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.0341311827272328\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
$EWB_pb=get_par("pb"); 
retlab7:;
$ax="utenti:user:pass";
$ax=ewb_remove($ax);
	$ax=$EWB_user;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="root";
	$bx=pop @as;
	$ax=($bx eq $ax);
	$bx=pop @as;
	$ax=$bx || $ax;
	if (!($ax)) { goto area110; };
	$ax="ok";
	$EWB_b=$ax;$ax="Utente non valido";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
    if (index($form, "name=\"pb\"") <0)
     { $form=$form.CreateForm("pb", $EWB_pb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 7+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.0407453652415697\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
$EWB_pb=get_par("pb"); 
retlab8:;
goto areaend110;
	
area110:;
	$ax="ok";
	$EWB_b=$ax;$ax="Utente cancellato";
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
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"infb\"") <0)
     { $form=$form.CreateForm("infb", $EWB_infb, "hidden");
     } 
    if (index($form, "name=\"pb\"") <0)
     { $form=$form.CreateForm("pb", $EWB_pb, "hidden");
     } 
$form=$form.CreateForm("EWjump", 8+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.629251140161077\">\
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
$EWB_inf=get_par("inf"); 
$EWB_b=get_par("b"); 
$EWB_infb=get_par("infb"); 
$EWB_pb=get_par("pb"); 
retlab9:;

areaend110:;
	goto areaend109;
	
area109:;
	;

areaend109:;
	goto area101;
	
area102:;
	goto areaend100;
	
area100:;
	$ax="Login non valido";
	$ax=print join(($ax), split("<>",$EWB_template));
	
areaend100:;
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

sub ewb_remove($)
{ my @t=split(":", $_[0]);
  $res="";
  my $code="\$res=tab_del(\"".$t[0]."\",\$EWB_".$t[1].");";
  eval($code);
  return $res;
};
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
