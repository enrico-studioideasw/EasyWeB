#!/usr/bin/perl
my $EWbuffer=<>;
print "Content-type: text/html\n\n"; 
#!/usr/bin/perl
my @EWlocked;
use IO::Socket;

my $EWB_template="<html><body bgcolor=lightyellow><div align=center><></body></html>";
my $EWB_format="12:5:h";
my $PROGRAM_NAME;
{ my @a=split('/', $0);
  $PROGRAM_NAME=$a[$#a];
}
my $rlist="";

sub get_par($)
{ my ($name)=@_;
  my $res="";
  my @values=split("&", $EWbuffer);
  foreach $val(@values)
  { my ($n, $val)=split(/=/, $val);
    if ($name eq EWconv($n)) { $res=EWconv($val);}
  } return $res;
}

#jumper: una funzione dinamica che descrive i punti di accesso.
if (get_par("EWjump") > 0)
{ jumper(get_par("EWjump"));
}
#funzioni predefinite
sub CreateForm
{ #nomecampo, valore, tipo campo, nome a video 
  my ($n,$v,$t,$vidname)=@_;
  if (($t ne "textarea")&&($t ne "info"))
  { $v=join("%22", (split "\"", $v));
    $v=join("%09", (split "\t", $v));
    $v=join("%10", (split "\n", $v));
    $v=join("%3C", (split "<", $v));
  }
  my @f=split(":", $EWB_format);
  if ($f[0]<1) {$f[0]=12; };
  if ($f[1]<1) {$f[1]=5; };

  my $res;
  my $vn=$vidname;
  if (length($vidname) <2) { $vidname=$n; };
  $res="<table border=0><tr><td>$vidname</td><td>";

  if (($t eq "submit") && (length($vn)<2)) { $res="<table border=0><tr><td></td><td>"; };
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
my $ax,$bx,$cx,$dx,@as;

my $EWB_a;

my $EWB_b;

my $EWB_c;

my $EWB_d;

my $EWB_e;
$ax="uno;*due;tre";
	$EWB_a=$ax;
	$ax="invia;annulla";
	$EWB_b=$ax;
	$ax="prima;seconda;terza;*selezionata;quarta";
	$EWB_d=$ax;
	
my $EWB_frm;
$ax=$EWB_format;
	push @as,$ax;
	$ax="<br>";
	$bx=pop @as;
	$ax=$bx.$ax;
	print $ax;
	$ax="<table border=1><tr><td>";
	$EWB_frm=$ax;
	$ax="40:6:v";
	$EWB_format=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	$ax="<font color=red>Seleziona:</font>";
	push @as,"a";
	push @as,$EWB_a;
	push @as,"radio";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$ax=CreateForm($ax,$bx,$cx,$dx);$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	$ax="</td><td>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	push @as,"c";
	push @as,$EWB_c;
	push @as,"text";
	push @as,"c";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$ax=CreateForm($ax,$bx,$cx,$dx);$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	$ax="</td></tr><tr><td>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax="8:6:v";
	$EWB_format=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	push @as,"d";
	push @as,$EWB_d;
	push @as,"option";
	push @as,"d";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$ax=CreateForm($ax,$bx,$cx,$dx);$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	$ax="</td><td>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax="10:8:v";
	$EWB_format=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	push @as,"e";
	push @as,$EWB_e;
	push @as,"textarea";
	push @as,"e";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$ax=CreateForm($ax,$bx,$cx,$dx);$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	$ax="</td></tr><tr><td>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax="40:6:h";
	$EWB_format=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$ax=CreateForm($ax,$bx,$cx,$dx);$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	$ax=$EWB_frm;
	push @as,$ax;
	$ax="</td><td></tr></table>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_frm=$ax;
	{ my $form=$EWB_frm."";
$EWB_frm=""; 
    if (index($form, "name=\"template\"") <0)
     { $form=$form.CreateForm("template", $EWB_template, "hidden");
     };
    if (index($form, "name=\"format\"") <0)
     { $form=$form.CreateForm("format", $EWB_format, "hidden");
     };
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     };
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     };
    if (index($form, "name=\"c\"") <0)
     { $form=$form.CreateForm("c", $EWB_c, "hidden");
     };
    if (index($form, "name=\"d\"") <0)
     { $form=$form.CreateForm("d", $EWB_d, "hidden");
     };
    if (index($form, "name=\"e\"") <0)
     { $form=$form.CreateForm("e", $EWB_e, "hidden");
     };
    if (index($form, "name=\"frm\"") <0)
     { $form=$form.CreateForm("frm", $EWB_frm, "hidden");
     };
$form=$form.CreateForm("EWjump", 0+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.782318900412978\">\
";
$form="<form name=\"ewb\" action=".$PROGRAM_NAME." method=post>".$form."</form>";
print join($form,split("<>", $EWB_template))."\
\
"; exit(0); 
}

goto retlab1;
lab1:
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_a=get_par("a"); 
$EWB_b=get_par("b"); 
$EWB_c=get_par("c"); 
$EWB_d=get_par("d"); 
$EWB_e=get_par("e"); 
$EWB_frm=get_par("frm"); 
retlab1:
exit(0); 

sub jumper($)
{ my $lab=$_[0];
if ($lab==1) { goto lab1; }
};

#Versione 3.16 Lajka
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
