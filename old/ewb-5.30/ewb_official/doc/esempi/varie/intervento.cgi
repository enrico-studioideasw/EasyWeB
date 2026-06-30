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
    { if (!(open (III, "< TB".$name.".lck")))
      { if (!(open (OOO, "> TB".$name.".lck")))
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
  if ($count=1) { unlink("TB".$name.".lck"); };
}

sub tab_read($)
{ #esegue la lettura di una tabella gia bloccata.
  my @t=split(":", $_[0]);
  open III,"< .TB$t[0].tab";
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
{ my ($n,$v,$t,$vidname)=@_;
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
    { $res=$res."<input type=radio name=\"$n\" value=\"$p\">$p $sep"; }
    if ($t eq "option")
    { $res=$res."<option value=\"$p\">$p"; }
    if ($t eq "submit")
    { $res=$res."<input name=\"$n\" type=submit value=\"$p\">$sep
"; }
  }
  if ($t eq "option") { $res=$res."</select>"; }
  if ($t ne "hidden") { $res=$res."</td></tr></table>"; }
  return $res."\
";
}

#liste - conversione da e a ascii
sub stpop($)
{  my @a=split(/ /,$_[0]);
   $res=$a[$#a];
   $_[0]=substr($_[0],0,length($_[0])-(length($res)+1));
   return fromasc($res);
}

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
my $ax,$bx,$cx,$dx,@as;
	my $TB_struttura="struttura:nomes:tels:citta:indirizzo";
	my $EWB_nomes=""; 
	my $EWB_tels=""; 
	my $EWB_citta=""; 
	my $EWB_indirizzo=""; 
		my $TB_referenti="referenti:nomecogn:datainser:telr:nomeassoc:ftpclient";
	my $EWB_nomecogn=""; 
	my $EWB_datainser=""; 
	my $EWB_telr=""; 
	my $EWB_nomeassoc=""; 
	my $EWB_ftpclient=""; 
		my $TB_rete="rete:nomer:nomecogn";
	my $EWB_nomer=""; 
	my $EWB_nomecogn=""; 
		my $TB_virtualhost="virtualhost:vnome";
	my $EWB_vnome=""; 
		my $TB_tipiintervento="tipiintervento:tipo";
	my $EWB_tipo=""; 
		my $TB_risorsaweb="risorsaweb:url:nomecogn:chiaveprog:user:user1:vnome";
	my $EWB_url=""; 
	my $EWB_nomecogn=""; 
	my $EWB_chiaveprog=""; 
	my $EWB_user=""; 
	my $EWB_user1=""; 
	my $EWB_vnome=""; 
		my $TB_accessi="accessi:user:password";
	my $EWB_user=""; 
	my $EWB_password=""; 
		my $TB_strumenti="strumenti:nomestrumento";
	my $EWB_nomestrumento=""; 
		my $TB_host="host:chiaveprog:ip:hostname:nomehost:sistop:proprietario:hhost:tecnologia:velocita";
	my $EWB_chiaveprog=""; 
	my $EWB_ip=""; 
	my $EWB_hostname=""; 
	my $EWB_nomehost=""; 
	my $EWB_sistop=""; 
	my $EWB_proprietario=""; 
	my $EWB_hhost=""; 
	my $EWB_tecnologia=""; 
	my $EWB_velocita=""; 
		my $TB_lavoraper="lavoraper:lvp";
	my $EWB_lvp=""; 
		my $TB_sitoweb="sitoweb:stw";
	my $EWB_stw=""; 
		my $TB_indirizzi="indirizzi:idr";
	my $EWB_idr=""; 
		my $TB_areaip="areaip:aip:rete:netmask:broadcast:dimensione";
	my $EWB_aip=""; 
	my $EWB_rete=""; 
	my $EWB_netmask=""; 
	my $EWB_broadcast=""; 
	my $EWB_dimensione=""; 
		my $TB_personale="personale:nomepers:telp:nomecogn:user";
	my $EWB_nomepers=""; 
	my $EWB_telp=""; 
	my $EWB_nomecogn=""; 
	my $EWB_user=""; 
		my $TB_interventi="interventi:intv:tipo:nomecogn:data2:ora2:chiaveprog:memo:esito";
	my $EWB_intv=""; 
	my $EWB_tipo=""; 
	my $EWB_nomecogn=""; 
	my $EWB_data2=""; 
	my $EWB_ora2=""; 
	my $EWB_chiaveprog=""; 
	my $EWB_memo=""; 
	my $EWB_esito=""; 
		my $TB_tecnologie="tecnologie:nome";
	my $EWB_nome=""; 
	$ax="open.html";
	$ax=EWload($ax);
	$EWB_template=$ax;
	
my $EWB_a;

my $EWB_i;

my $EWB_b;
$ax=1;
	$EWB_i=$ax;
	$ax="Intervento svolto";
	$EWB_b=$ax;
	$ax="Interventi pendenti<br></b><table border>";
	$EWB_a=$ax;
	
my $EWB_v;
$ax="";
	$EWB_v=$ax;
	$ax="referenti:nomecogn:datainser:telr:nomeassoc:ftpclient";
  $tot=$ax;
  $n=((split(":",$tot))[0]);
  @p=split(":",substr($tot,length($n)+1));
  tab_wlock($n);
  my @f=tab_read($tot);
  tab_unlock($n);
  my $l; my $i=0;
  my $g=0;
area100:;
  if ($g > $#f) { goto area102; };
  $l=$f[$g]; $l=substr($l,0,length($l));
  my $k;
  my $found=0;
  $i++;
  for ($k=$i; $k<=$#f; $k++)
  { if ( ((split( "#",$l))[0]) eq ((split("#",$f[$k]))[0]) )
    { $found=1; };
  }
  if ($found) { goto area101;};
  my @aa=split("#", $l);
  my $ct=0;
  $code="";
  foreach $p(@p)
  { $code=$code."\$EWB_".$p."=addchar(\$aa[$ct]);
";
    $ct++;
  };
  eval($code);
	$ax=$EWB_v;
	push @as,$ax;
	$ax=$EWB_nomecogn;
	push @as,$ax;
	$ax=";";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_v=$ax;
	
area101:;
  $g=$g+1;
  goto area100;
area102:;
	
my $EWB_nomepers;

my $EWB_data1;

my $EWB_ora1;
$ax="interventi:intv:tipo:nomecogn:data2:ora2:chiaveprog:memo:esito";
  $tot=$ax;
  $n=((split(":",$tot))[0]);
  @p=split(":",substr($tot,length($n)+1));
  tab_wlock($n);
  my @f=tab_read($tot);
  tab_unlock($n);
  my $l; my $i=0;
  my $g=0;
area103:;
  if ($g > $#f) { goto area105; };
  $l=$f[$g]; $l=substr($l,0,length($l));
  my $k;
  my $found=0;
  $i++;
  for ($k=$i; $k<=$#f; $k++)
  { if ( ((split( "#",$l))[0]) eq ((split("#",$f[$k]))[0]) )
    { $found=1; };
  }
  if ($found) { goto area104;};
  my @aa=split("#", $l);
  my $ct=0;
  $code="";
  foreach $p(@p)
  { $code=$code."\$EWB_".$p."=addchar(\$aa[$ct]);
";
    $ct++;
  };
  eval($code);
	$ax=$EWB_nomecogn;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area104;};
	$ax=stpop($EWB_intv);
	$EWB_ora1=$ax;
	$ax=stpop($EWB_intv);
	$EWB_data1=$ax;
	$ax=stpop($EWB_intv);
	$EWB_nomepers=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<tr><td>";
	push @as,$ax;
	$ax=$EWB_i;
	push @as,$ax;
	$ax="</td><td>";
	push @as,$ax;
	$ax=$EWB_nomepers;
	push @as,$ax;
	$ax="</td><td>";
	push @as,$ax;
	$ax=$EWB_data1;
	push @as,$ax;
	$ax=", ";
	push @as,$ax;
	$ax=$EWB_ora1;
	push @as,$ax;
	$ax="</td><td>";
	push @as,$ax;
	$ax=$EWB_tipo;
	push @as,$ax;
	$ax="</td><td>";
	push @as,$ax;
	$ax=$EWB_memo;
	push @as,$ax;
	$ax="</td><td>";
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
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax="host:chiaveprog:ip:hostname:nomehost:sistop:proprietario:hhost:tecnologia:velocita";
$ax=ewb_exist($ax);
	$ax=$EWB_a;
	push @as,$ax;
	$ax=$EWB_ip;
	push @as,$ax;
	$ax=", ";
	push @as,$ax;
	$ax=$EWB_proprietario;
	push @as,$ax;
	$ax="</td></tr>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_i;
	push @as,$ax;
	$ax=1;
	$bx=pop @as;
	$ax=$bx + $ax;
	$EWB_i=$ax;
	
area104:;
  $g=$g+1;
  goto area103;
area105:;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="</table><br>";
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	
my $EWB_n;
$ax="";
	$EWB_n=$ax;
	$ax="";
	$EWB_esito=$ax;
	my $form="";
	push @as,"a";
	push @as,$EWB_a;
	push @as,"info";
	push @as,"a";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="numero";
	push @as,"n";
	push @as,$EWB_n;
	push @as,"text";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	$ax="referente";
	push @as,"v";
	push @as,$EWB_v;
	push @as,"option";
	push @as,$ax;
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"esito";
	push @as,$EWB_esito;
	push @as,"text";
	push @as,"esito";
	$dx=pop @as;
	$cx=pop @as;
	$bx=pop @as;
	$ax=pop @as;
	$form.=CreateForm($ax,$bx,$cx,$dx);
	push @as,"b";
	push @as,$EWB_b;
	push @as,"submit";
	push @as,"b";
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
    if (index($form, "name=\"nomes\"") <0)
     { $form=$form.CreateForm("nomes", $EWB_nomes, "hidden");
     } 
    if (index($form, "name=\"tels\"") <0)
     { $form=$form.CreateForm("tels", $EWB_tels, "hidden");
     } 
    if (index($form, "name=\"citta\"") <0)
     { $form=$form.CreateForm("citta", $EWB_citta, "hidden");
     } 
    if (index($form, "name=\"indirizzo\"") <0)
     { $form=$form.CreateForm("indirizzo", $EWB_indirizzo, "hidden");
     } 
    if (index($form, "name=\"nomecogn\"") <0)
     { $form=$form.CreateForm("nomecogn", $EWB_nomecogn, "hidden");
     } 
    if (index($form, "name=\"datainser\"") <0)
     { $form=$form.CreateForm("datainser", $EWB_datainser, "hidden");
     } 
    if (index($form, "name=\"telr\"") <0)
     { $form=$form.CreateForm("telr", $EWB_telr, "hidden");
     } 
    if (index($form, "name=\"nomeassoc\"") <0)
     { $form=$form.CreateForm("nomeassoc", $EWB_nomeassoc, "hidden");
     } 
    if (index($form, "name=\"ftpclient\"") <0)
     { $form=$form.CreateForm("ftpclient", $EWB_ftpclient, "hidden");
     } 
    if (index($form, "name=\"nomer\"") <0)
     { $form=$form.CreateForm("nomer", $EWB_nomer, "hidden");
     } 
    if (index($form, "name=\"nomecogn\"") <0)
     { $form=$form.CreateForm("nomecogn", $EWB_nomecogn, "hidden");
     } 
    if (index($form, "name=\"vnome\"") <0)
     { $form=$form.CreateForm("vnome", $EWB_vnome, "hidden");
     } 
    if (index($form, "name=\"tipo\"") <0)
     { $form=$form.CreateForm("tipo", $EWB_tipo, "hidden");
     } 
    if (index($form, "name=\"url\"") <0)
     { $form=$form.CreateForm("url", $EWB_url, "hidden");
     } 
    if (index($form, "name=\"nomecogn\"") <0)
     { $form=$form.CreateForm("nomecogn", $EWB_nomecogn, "hidden");
     } 
    if (index($form, "name=\"chiaveprog\"") <0)
     { $form=$form.CreateForm("chiaveprog", $EWB_chiaveprog, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"user1\"") <0)
     { $form=$form.CreateForm("user1", $EWB_user1, "hidden");
     } 
    if (index($form, "name=\"vnome\"") <0)
     { $form=$form.CreateForm("vnome", $EWB_vnome, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"password\"") <0)
     { $form=$form.CreateForm("password", $EWB_password, "hidden");
     } 
    if (index($form, "name=\"nomestrumento\"") <0)
     { $form=$form.CreateForm("nomestrumento", $EWB_nomestrumento, "hidden");
     } 
    if (index($form, "name=\"chiaveprog\"") <0)
     { $form=$form.CreateForm("chiaveprog", $EWB_chiaveprog, "hidden");
     } 
    if (index($form, "name=\"ip\"") <0)
     { $form=$form.CreateForm("ip", $EWB_ip, "hidden");
     } 
    if (index($form, "name=\"hostname\"") <0)
     { $form=$form.CreateForm("hostname", $EWB_hostname, "hidden");
     } 
    if (index($form, "name=\"nomehost\"") <0)
     { $form=$form.CreateForm("nomehost", $EWB_nomehost, "hidden");
     } 
    if (index($form, "name=\"sistop\"") <0)
     { $form=$form.CreateForm("sistop", $EWB_sistop, "hidden");
     } 
    if (index($form, "name=\"proprietario\"") <0)
     { $form=$form.CreateForm("proprietario", $EWB_proprietario, "hidden");
     } 
    if (index($form, "name=\"hhost\"") <0)
     { $form=$form.CreateForm("hhost", $EWB_hhost, "hidden");
     } 
    if (index($form, "name=\"tecnologia\"") <0)
     { $form=$form.CreateForm("tecnologia", $EWB_tecnologia, "hidden");
     } 
    if (index($form, "name=\"velocita\"") <0)
     { $form=$form.CreateForm("velocita", $EWB_velocita, "hidden");
     } 
    if (index($form, "name=\"lvp\"") <0)
     { $form=$form.CreateForm("lvp", $EWB_lvp, "hidden");
     } 
    if (index($form, "name=\"stw\"") <0)
     { $form=$form.CreateForm("stw", $EWB_stw, "hidden");
     } 
    if (index($form, "name=\"idr\"") <0)
     { $form=$form.CreateForm("idr", $EWB_idr, "hidden");
     } 
    if (index($form, "name=\"aip\"") <0)
     { $form=$form.CreateForm("aip", $EWB_aip, "hidden");
     } 
    if (index($form, "name=\"rete\"") <0)
     { $form=$form.CreateForm("rete", $EWB_rete, "hidden");
     } 
    if (index($form, "name=\"netmask\"") <0)
     { $form=$form.CreateForm("netmask", $EWB_netmask, "hidden");
     } 
    if (index($form, "name=\"broadcast\"") <0)
     { $form=$form.CreateForm("broadcast", $EWB_broadcast, "hidden");
     } 
    if (index($form, "name=\"dimensione\"") <0)
     { $form=$form.CreateForm("dimensione", $EWB_dimensione, "hidden");
     } 
    if (index($form, "name=\"nomepers\"") <0)
     { $form=$form.CreateForm("nomepers", $EWB_nomepers, "hidden");
     } 
    if (index($form, "name=\"telp\"") <0)
     { $form=$form.CreateForm("telp", $EWB_telp, "hidden");
     } 
    if (index($form, "name=\"nomecogn\"") <0)
     { $form=$form.CreateForm("nomecogn", $EWB_nomecogn, "hidden");
     } 
    if (index($form, "name=\"user\"") <0)
     { $form=$form.CreateForm("user", $EWB_user, "hidden");
     } 
    if (index($form, "name=\"intv\"") <0)
     { $form=$form.CreateForm("intv", $EWB_intv, "hidden");
     } 
    if (index($form, "name=\"tipo\"") <0)
     { $form=$form.CreateForm("tipo", $EWB_tipo, "hidden");
     } 
    if (index($form, "name=\"nomecogn\"") <0)
     { $form=$form.CreateForm("nomecogn", $EWB_nomecogn, "hidden");
     } 
    if (index($form, "name=\"data2\"") <0)
     { $form=$form.CreateForm("data2", $EWB_data2, "hidden");
     } 
    if (index($form, "name=\"ora2\"") <0)
     { $form=$form.CreateForm("ora2", $EWB_ora2, "hidden");
     } 
    if (index($form, "name=\"chiaveprog\"") <0)
     { $form=$form.CreateForm("chiaveprog", $EWB_chiaveprog, "hidden");
     } 
    if (index($form, "name=\"memo\"") <0)
     { $form=$form.CreateForm("memo", $EWB_memo, "hidden");
     } 
    if (index($form, "name=\"esito\"") <0)
     { $form=$form.CreateForm("esito", $EWB_esito, "hidden");
     } 
    if (index($form, "name=\"nome\"") <0)
     { $form=$form.CreateForm("nome", $EWB_nome, "hidden");
     } 
    if (index($form, "name=\"a\"") <0)
     { $form=$form.CreateForm("a", $EWB_a, "hidden");
     } 
    if (index($form, "name=\"i\"") <0)
     { $form=$form.CreateForm("i", $EWB_i, "hidden");
     } 
    if (index($form, "name=\"b\"") <0)
     { $form=$form.CreateForm("b", $EWB_b, "hidden");
     } 
    if (index($form, "name=\"v\"") <0)
     { $form=$form.CreateForm("v", $EWB_v, "hidden");
     } 
    if (index($form, "name=\"nomepers\"") <0)
     { $form=$form.CreateForm("nomepers", $EWB_nomepers, "hidden");
     } 
    if (index($form, "name=\"data1\"") <0)
     { $form=$form.CreateForm("data1", $EWB_data1, "hidden");
     } 
    if (index($form, "name=\"ora1\"") <0)
     { $form=$form.CreateForm("ora1", $EWB_ora1, "hidden");
     } 
    if (index($form, "name=\"n\"") <0)
     { $form=$form.CreateForm("n", $EWB_n, "hidden");
     } 
$form=$form.CreateForm("EWjump", 0+1, "hidden"); 

$form=$form."<input type=hidden name=\"EWB_Hid_FIELD\" value=\"0.327178955078125\">\
";
$form="<form action=".$PROGRAM_NAME." method=post>".$form."</form>";
print join($form,split("<>", $EWB_template)); exit(0);
	
goto retlab1;
lab1:
$EWB_template=get_par("template"); 
$EWB_format=get_par("format"); 
$EWB_nomes=get_par("nomes"); 
$EWB_tels=get_par("tels"); 
$EWB_citta=get_par("citta"); 
$EWB_indirizzo=get_par("indirizzo"); 
$EWB_nomecogn=get_par("nomecogn"); 
$EWB_datainser=get_par("datainser"); 
$EWB_telr=get_par("telr"); 
$EWB_nomeassoc=get_par("nomeassoc"); 
$EWB_ftpclient=get_par("ftpclient"); 
$EWB_nomer=get_par("nomer"); 
$EWB_nomecogn=get_par("nomecogn"); 
$EWB_vnome=get_par("vnome"); 
$EWB_tipo=get_par("tipo"); 
$EWB_url=get_par("url"); 
$EWB_nomecogn=get_par("nomecogn"); 
$EWB_chiaveprog=get_par("chiaveprog"); 
$EWB_user=get_par("user"); 
$EWB_user1=get_par("user1"); 
$EWB_vnome=get_par("vnome"); 
$EWB_user=get_par("user"); 
$EWB_password=get_par("password"); 
$EWB_nomestrumento=get_par("nomestrumento"); 
$EWB_chiaveprog=get_par("chiaveprog"); 
$EWB_ip=get_par("ip"); 
$EWB_hostname=get_par("hostname"); 
$EWB_nomehost=get_par("nomehost"); 
$EWB_sistop=get_par("sistop"); 
$EWB_proprietario=get_par("proprietario"); 
$EWB_hhost=get_par("hhost"); 
$EWB_tecnologia=get_par("tecnologia"); 
$EWB_velocita=get_par("velocita"); 
$EWB_lvp=get_par("lvp"); 
$EWB_stw=get_par("stw"); 
$EWB_idr=get_par("idr"); 
$EWB_aip=get_par("aip"); 
$EWB_rete=get_par("rete"); 
$EWB_netmask=get_par("netmask"); 
$EWB_broadcast=get_par("broadcast"); 
$EWB_dimensione=get_par("dimensione"); 
$EWB_nomepers=get_par("nomepers"); 
$EWB_telp=get_par("telp"); 
$EWB_nomecogn=get_par("nomecogn"); 
$EWB_user=get_par("user"); 
$EWB_intv=get_par("intv"); 
$EWB_tipo=get_par("tipo"); 
$EWB_nomecogn=get_par("nomecogn"); 
$EWB_data2=get_par("data2"); 
$EWB_ora2=get_par("ora2"); 
$EWB_chiaveprog=get_par("chiaveprog"); 
$EWB_memo=get_par("memo"); 
$EWB_esito=get_par("esito"); 
$EWB_nome=get_par("nome"); 
$EWB_a=get_par("a"); 
$EWB_i=get_par("i"); 
$EWB_b=get_par("b"); 
$EWB_v=get_par("v"); 
$EWB_nomepers=get_par("nomepers"); 
$EWB_data1=get_par("data1"); 
$EWB_ora1=get_par("ora1"); 
$EWB_n=get_par("n"); 
retlab1:
$ax=$EWB_n;
	push @as,$ax;
	$ax=0;
	$bx=pop @as;
	$ax=($bx < $ax);
	push @as,$ax;
	$ax=$EWB_n;
	push @as,$ax;
	$ax=$EWB_i;
	$bx=pop @as;
	$ax=($bx >= $ax);
	$bx=pop @as;
	$ax=$bx || $ax;
	if (!($ax)) { goto area106; };
	$ax="Numero non in elenco";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit ($ax);
	goto areaend106;
	
area106:;
	;

areaend106:;
	$ax=$EWB_v;
	push @as,$ax;
	$ax="";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area107; };
	$ax="Referente nullo.";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit ($ax);
	goto areaend107;
	
area107:;
	;

areaend107:;
	$ax=1;
	$EWB_i=$ax;
	$ax="interventi:intv:tipo:nomecogn:data2:ora2:chiaveprog:memo:esito";
  $tot=$ax;
  $n=((split(":",$tot))[0]);
  @p=split(":",substr($tot,length($n)+1));
  tab_wlock($n);
  my @f=tab_read($tot);
  tab_unlock($n);
  my $l; my $i=0;
  my $g=0;
area108:;
  if ($g > $#f) { goto area110; };
  $l=$f[$g]; $l=substr($l,0,length($l));
  my $k;
  my $found=0;
  $i++;
  for ($k=$i; $k<=$#f; $k++)
  { if ( ((split( "#",$l))[0]) eq ((split("#",$f[$k]))[0]) )
    { $found=1; };
  }
  if ($found) { goto area109;};
  my @aa=split("#", $l);
  my $ct=0;
  $code="";
  foreach $p(@p)
  { $code=$code."\$EWB_".$p."=addchar(\$aa[$ct]);
";
    $ct++;
  };
  eval($code);
	$ax=$EWB_i;
	push @as,$ax;
	$ax=$EWB_n;
	$bx=pop @as;
	$ax=($bx == $ax);
	if (!($ax)) { goto area111; };
	$ax="trovato intervento.";
	print $ax;
	$ax=$EWB_v;
	$EWB_nomecogn=$ax;
	$ax=(EWdate ());
	$EWB_data2=$ax;
	$ax=(EWtime ());
	$EWB_ora2=$ax;
	$ax="interventi:intv:tipo:nomecogn:data2:ora2:chiaveprog:memo:esito";
$ax=ewb_add($ax);
	$ax="Esito inserito.";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit ($ax);
	goto areaend111;
	
area111:;
	;

areaend111:;
	$ax=$EWB_i;
	push @as,$ax;
	$ax=1;
	$bx=pop @as;
	$ax=$bx + $ax;
	$EWB_i=$ax;
	
area109:;
  $g=$g+1;
  goto area108;
area110:;
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
if ($lab==1) { goto lab1; }
};

#Versione 3.11 pitbull
#Enrico Betti e Paride Dominici



sub ewb_exist($)
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
  $cond=$cond.")=tab_exist(\"$tot[0]\",\$EWB_";
  $cond=$cond.$tot[1].")));";
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
if (!(open oof,\">> .TB$tot[0].tab\"))
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
  $a=~ tr/+/ /;
  $a=~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C",hex($1))/eg;
  $a=join("
", split("%10", $a));
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

sub tab_exist($$)
{ my ($n,$k)=@_;
  tab_wlock($n);
  open III,"<.TB$n.tab";
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

sub fromasc($)
{ my $a=$_[0];
  my $r; my $i;
  for($i=0; $i<length($a); $i=$i+2)
  { $r=$r.chr(hex(substr($a,$i,2)));
  } return $r;
}
