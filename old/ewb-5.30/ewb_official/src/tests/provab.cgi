#!/usr/bin/perl
print "Content-type: text/html\n\n"; 
#!/usr/bin/perl
my @EWlocked;
use IO::Socket;
my @fltype;
my $EWB_tabpath="./";
my $EWB_template="<html><body bgcolor=lightyellow><div align=center><></body></html>";
my $EWB_format="12:5:h";
my $EWB_sqldbase="easyweb_db";
my $EWB_sqlserver="";
my $EWB_sqluser="";
my $EWB_sqlpassword="";
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
sub match($$$)
{my ($clausea,$clauseb, $vars)=@_;
  my $a=funtore $clausea;
  my $az=parsepart $clausea;
  $clausea=substr($clausea, length($a));
  my $b=funtore $clauseb;
  my $bz=parsepart $clauseb;
  $clauseb=substr($clauseb, length($b));
  my $res="";

  if (!isvariable($a) && !isvariable($b))
  { #devono corrispondere i funtori e matchare gli argomenti.
    if ($a eq $b)
    { $res=1;
      if ( ($a ne $az)&&(substr($clauseb,0,1) eq "(")&&(substr($clausea,0,1) eq "(") )
      { $res=1;
        #pesco gli argomenti e ne richiedo il match..
        $clausea=",".substr($clausea,1);
        $clauseb=",".substr($clauseb,1);
        do
        { $clausea=substr($clausea,1);
          $clauseb=substr($clauseb,1);
	  if (($clausea ne ")") && ($clauseb ne ")"))
	  { if  (match($clausea,$clauseb,$vars)ne 1)
	    {$res=0;};
	  }
#qui tolgo la clausola di testa.
        $clausea=substr($clausea,length(parsepart($clausea)));
        $clauseb=substr($clauseb,length(parsepart($clauseb)));
        } while ((substr($clausea,0,1) eq ",") && (substr($clauseb,0,1) eq ","));
        $res=$res && match($clausea,$clauseb,$vars);
        if ((substr($clausea,0,1) ne ")" ) || (substr($clauseb,0,1) ne ")"))
        { $res=0;
        }
        if ($res eq 1)
        { $clausea=substr($clausea,0,1);
          $clauseb=substr($clauseb,0,1);
        }
      }
    }
  }
  elsif (isvariable($a) && isvariable($b))
  { if ((varval($a,$vars)eq "<Null>") && (varval($b,$vars) eq "<Null>"))
    { varassign $a,$b, $vars; $res=1;
    }
    elsif ((varval($a,$vars)eq "<Null>") && (varval($b,$vars) ne "<Null>"))
    { varassign($a,varval($b,$vars), $vars); $res=1;
    }
    elsif ((varval($a,$vars) ne "<Null>") && (varval($b,$vars) eq "<Null>"))
    { varassign($b, varval($a,$vars),$vars); $res=1;
    }
    else
    { $res=match(varval($a,$vars), varval($b,$vars), $vars);
    }

  }
  elsif (isvariable($a) && !isvariable($b))
  { if (varval($a,$vars) eq "<Null>")
    { varassign($a, $bz,$vars); $res=1;
    }
    else {
    $res=match(varval($a,$vars),$bz, $vars);
    }
  }
  elsif (!isvariable($a) && isvariable($b))
  { if (varval($b,$vars) eq "<Null>")
    { varassign($b, $az,$vars); $res=1;
    }
    else {$res=match($az,varval($b,$vars), $vars); }
  }
  #effetto collaterale. Cambio le variabili passate per valore.
  if ($res) { $_[2]=$vars; }
  return $res;
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

my $EWB_a;
	
my $EWB_b;
	
my $EWB_c;
	
my $EWB_d;
	$ax="ok";
	$EWB_c=$ax;$ax="uno;*due;tre";
	$EWB_b=$ax;$ax="alfa;beta;*gamma;*delta";
	$EWB_a=$ax;$ax="12:5:v";
	$EWB_format=$ax;my $form="";
	push @as,"a";
	push @as,$EWB_a;
	push @as,"options";
	push @as,"a";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"options";
	push @as,"b";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"c";
	push @as,$EWB_c;
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
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"c\"") <0)
     { $form=$form.CreateForm("c", $EWB_c, "hidden");
     } 
    if (index($form, "name=\"d\"") <0)
     { $form=$form.CreateForm("d", $EWB_d, "hidden");
     } 
$form=$form.CreateForm("EWjump", 0+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.735480548654781\">\
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
$EWB_a=get_par("a"); 
$EWB_b=get_par("b"); 
$EWB_c=get_par("c"); 
$EWB_d=get_par("d"); 
retlab1:;
$ax="A: [";
	push @as,$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="]<br>
B: [";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="]<br>";
	$bx=pop @as;
	$ax=$bx.$ax;
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

#Amethist easyweb v. 5.17 formatchecks
#Enrico Betti e Paride Dominici



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

sub funtore($)
{ #torna la parte della stringa composta di caratteri a..z, A..Z, 1..9, _
  my $l=$_[0];
  my $a;
  my $r;
  for ($a=substr($l,0,1);
        ( ((uc($a) ge "A") && (uc($a) le "Z")) || (($a ge"0") &&  ($a le "9")) || ($a eq "_") || 
($a eq "~"));
        $a=substr($l,0, 1) )
        { $r=$r.$a;
           $l=substr($l,1);
        };
  return $r;
}

sub parsepart($)
{ #torna il funtore seguito dall' eventuale aperta parentesi,e da
  #un conteggio finche' non si chiude.
  my $l=$_[0];
  my $a;
  my $r = funtore $l;
  $l=substr($l,length($r));
  if (substr($l,0,1) eq "(")
  { #conteggio di parentesi.. avere una macchina di touring..
     my $cp=1; $l=substr($l,1);
     $r=$r."(";
     for($a=substr($l,0,1); $cp>0; $a=substr($l,0,1) )
     { $l=substr($l,1);
        if (($l eq "")&&($cp>1)) {die "$cp fine stringa inaspettato.";};
        if ($a eq "(") {$cp++};
        if ($a eq ")") {$cp--; };
        $r=$r.$a;
     }
  }
  return $r;
}

sub isvariable($)
{ my $a=substr($_[0],0,1);
  return (($a ge 'A') && ($a le 'Z')) || ($a eq "_");
}

sub varassign($$$)
{ #ricorda l' associazione nome=valore.
  #valore puo' essere il nome di un altra variabile.Non della stessa.
  my ($a,$b, $vars)=@_;
  if ($a ne $b)
  { my $n=$a."#".$b;
    $vars=$vars.$n."\n";
    $_[2]=$vars;
  }
}

sub varval($$)
{ #legge il valore di una variabile.
  #se e' il valore di un altra variabile si richiama in ricorsivo.
  my ($a, $vars)=@_;
  my $l;
  my @vars=split("\n",$vars);
  my $res="<Null>";
  foreach $l(@vars)
  { my($name,$val)=split("#",$l);
    if ($name eq $a) {$res = $val;};
  }
  my $aa=substr($res,0,1);
  if ((($aa ge "A") && ($aa le "Z")) || ($a eq "_"))
  { $res=varval($res,$vars);
  }
  return $res;
}
