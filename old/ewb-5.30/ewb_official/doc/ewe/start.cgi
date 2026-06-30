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
	
my $EWB_b;
	
my $EWB_inf;
	$ax="Ok";
	$EWB_b=$ax;
my $EWB_lnk;
	$ax="<br><br>
</b>
<a href=admin.cgi>Amministra gli utenti</a>
<b>";
	$EWB_lnk=$ax;$ax="Login al servizio amethyst editor";
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
	$ax="Utente";
	push @as,"user";
	push @as,$EWB_user;
	push @as,"text";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="Password";
	push @as,"pass";
	push @as,$EWB_pass;
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
	push @as,"lnk";
	push @as,$EWB_lnk;
	push @as,"info";
	push @as,"lnk";
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
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"inf\"") <0)
     { $form=$form.CreateForm("inf", $EWB_inf, "hidden");
     } 
    if (index($form, "name=\"lnk\"") <0)
     { $form=$form.CreateForm("lnk", $EWB_lnk, "hidden");
     } 
$form=$form.CreateForm("EWjump", 0+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.536296394960122\">\
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
$EWB_b=get_par("b"); 
$EWB_inf=get_par("inf"); 
$EWB_lnk=get_par("lnk"); 
retlab1:;
$ax="utenti:user:pass";
$ax=ewb_login($ax);
	$ax=!$ax;
	if (!($ax)) { goto area100; };
	$ax="Accesso non valido";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit (0);
	goto areaend100;
	
area100:;
	;

areaend100:;
	$ax="pages/start.html";
	$ax=EWload($ax);
	$EWB_template=$ax;$ax="?";
	push @as,$ax;
	$ax=$EWB_user;
	push @as,$ax;
	$ax="&";
	push @as,$ax;
	$ax=$EWB_pass;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
if ($lab==1) { goto lab1; }
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

sub checkpath()
{ my $a=$EWB_tabpath;
  if (substr($a,length($a)-1,1) ne "/")
  { $a=$a."/";
  }; return $a;
};

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

sub delchar($)
{ my $a=join("<EWB_ASTErISCO>",split "#", $_[0]);
  $a=join("<EWB_LineFEEd>", split(chr(0x0D), $a));
  return join("<EWB_CARRIaGErETURN>", split("\n", $a));
}

sub addchar($)
{ my $a=join("#",split "<EWB_ASTErISCO>", $_[0]);
  $a=join(chr(0x0D),split("<EWB_LineFEEd>", $a));
  return join("\n", split("<EWB_CARRIaGErETURN>", $a));
}

sub tab_islock($)
{ my $name=$_[0];
  my $l; my $r=0;
  foreach $l(@EWlocked)
  { if ($l eq $name) {$r=1;}
  } return $r;
}
