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
  open III,"<.TB$n.tab";
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
  open oof,">.TB$n.tab";
  foreach $l(@a)
  { print oof $l;
  }
  close oof; tab_unlock($n); return $r;
}
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
		my $TB_interventi="interventi:intv:tipo:nomecogn:data2:ora2:chiaveprog:memo";
	my $EWB_intv=""; 
	my $EWB_tipo=""; 
	my $EWB_nomecogn=""; 
	my $EWB_data2=""; 
	my $EWB_ora2=""; 
	my $EWB_chiaveprog=""; 
	my $EWB_memo=""; 
	$ax="tipiintervento:tipo";
push @as,$ax;
	$ax="tipo";
	$bx=pop @as;
	$ax=tab_sort($bx,$ax);exit(0); 

sub jumper($)
{ my $lab=$_[0];
};

#Versione 3.07 spaniel
#Enrico Betti e Paride Dominici


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

sub tab_islock($)
{ my $name=$_[0];
  my $l; my $r=0;
  foreach $l(@EWlocked)
  { if ($l eq $name) {$r=1;}
  } return $r;
}
