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
      $v=join("%10", (split "\n", $v));
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
sub EWsocket($$)
{ my $remote= IO::Socket::INET->new( proto => "tcp",
                                     PeerAddr => $_[0],
                                     PeerPort => $_[1]);
  if ($remote) { $remote->autoflush(1); };
  return $remote;
}

sub EWsread($)
{ my $a=$_[0];
  $a=<$a>; 
  return $a;
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

my $EWB_sk;

my $EWB_a;

my $EWB_b;

my $EWB_i;

my $EWB_rd;
$ax="Go";
	$EWB_sk=$ax;
	my $form="";
	push @as,"a";
	push @as,$EWB_a;
	push @as,"text";
	push @as,"a";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"sk";
	push @as,$EWB_sk;
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
    if (index($form, "name=\"sk\"") <0)
     { $form=$form.CreateForm("sk", $EWB_sk, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"i\"") <0)
     { $form=$form.CreateForm("i", $EWB_i, "hidden");
     } 
    if (index($form, "name=\"rd\"") <0)
     { $form=$form.CreateForm("rd", $EWB_rd, "hidden");
     } 
$form=$form.CreateForm("EWjump", 0+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.63216070372528\">\
";
$form="<form action=".$PROGRAM_NAME." method=post enctype=\"multipart/form-data\">".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab1;
lab1:;
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_tabpath=get_par("tabpath"); 
$EWB_sk=get_par("sk"); 
$EWB_a=get_par("a"); 
$EWB_b=get_par("b"); 
$EWB_i=get_par("i"); 
$EWB_rd=get_par("rd"); 
retlab1:;
$ax=$EWB_a;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$EWB_i=$ax;
	$ax=$EWB_i;
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx > $ax);
	if (!($ax)) { goto area100; };
	$ax=$EWB_a;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_a;
	push @as,$ax;
	$ax=$EWB_i;
	push @as,$ax;
	$ax=1;
	$bx=pop @as;
	$ax=$bx + $ax;
	push @as,$ax;
	$ax=$EWB_a;
	$ax=length($ax);
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$EWB_b=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="/";
	$bx=pop @as;
	$ax=index($bx,$ax);
	$ax=$EWB_a;
	push @as,$ax;
	$ax=$EWB_i;
	push @as,$ax;
	$ax=1;
	$bx=pop @as;
	$ax=$bx + $ax;
	push @as,$ax;
	$ax=$EWB_a;
	$ax=length($ax);
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$ax=$EWB_a;
	push @as,$ax;
	$ax=0;
	push @as,$ax;
	$ax=$EWB_i;
	push @as,$ax;
	$ax=1;
	$bx=pop @as;
	$ax=$bx - $ax;
	$bx=pop @as;
	$cx=pop@as;
	$ax=substr($cx,$bx,$ax);
	$EWB_a=$ax;
	goto areaend100;
	
area100:;
	;

areaend100:;
	$ax="a vale [";
	push @as,$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="] </n>b vale [";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="]\n";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	print $ax;
	$ax=$EWB_a;
	push @as,$ax;$ax=80;
	$bx=pop @as;
	$ax=EWsocket($bx,$ax);
	$EWB_sk=$ax;
	$ax=$EWB_sk;
	push @as,$ax;
	$ax="GET /";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax="/\n\n";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=print $bx $ax;
	$ax=" ";
	$EWB_a=$ax;
	$ax="";
	$EWB_rd=$ax;
	
area101:;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx ne $ax);
	if (! ($ax)) {goto area102;};
	$ax=$EWB_sk;
	$ax=EWsread($ax);
	$EWB_a=$ax;
	$ax=$EWB_rd;
	push @as,$ax;
	$ax=$EWB_a;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_rd=$ax;
	goto area101;
	
area102:;
	$ax=$EWB_sk;
	$ax=close($ax);
	$ax=$EWB_rd;
	print $ax;
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
if ($lab==1) { goto lab1; }
};

#Versione 4.10 - Gold
#Enrico Betti e Paride Dominici



sub EWconv($)
{ my $a=$_[0];
  $a=join("%3C", split("%253C", $a));
  $a=~ tr/+/ /;
  $a=~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C",hex($1))/eg;
  $a=join("
", split("%10", $a));
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
