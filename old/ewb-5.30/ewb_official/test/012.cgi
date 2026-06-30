#!/usr/bin/perl
my @EWlocked;
my $EWbuffer;
use IO::Socket;

my $EWB_template="<html><body bgcolor=lightyellow><div align=center><></body></html>";
my $EWB_format="12:5:h";
my $PROGRAM_NAME;
{ my @a=split('/', $0);
  $PROGRAM_NAME=$a[$#a];
}
my $rlist="";

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
  open III,"< $t[0]";
    my $k=join("",<III>);
    my $r=split("\n",$k);
  close III;
  return $r;
};
sub addchar($)
{ my $a=join("#",split "<EWB_ASTErISCO>", $_[0]);
  $a=join(chr(0x0D),split("<EWB_LineFEEd>", $a));
  return join("\n", split("<EWB_CARRIaGErETURN>", $a));
}


	$EWbuffer=<>;
	print "Content-type: text/html\n\n"; 
	my $ax,$bx,$cx,$dx,@as;

###	table tb=a,b,c,d;
		my $TB_tb="tb:a:b:c:d";
	my $EWB_a=""; 
	my $EWB_b=""; 
	my $EWB_c=""; 
	my $EWB_d=""; 
	
###	
	
###	in(tb,b eq "bb")
	$ax="tb:a:b:c:d";
  $tot=$ax;
  $n=((split(":",$tot))[0]);
  @p=split(":",substr($tot,length($n)+1));
  tab_wlock($n);
  my @f=tab_read($tot);
  tab_unlock($n);
  my $l; my $i=0;
  my $g=0;
area100:
  if ($g > $#f) { goto area102; };
  $l=$f[$g]; $l=substr($l,0,length($l)-1);
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
	$ax=$EWB_b;
	push @as,$ax;
	$ax="bb";
	$bx=pop @as;
	$ax=($bx eq $ax);
	if (!($ax)) { goto area101;};
	$ax=$EWB_a;
	push @as,$ax;
	$ax=".";
	push @as,$ax;
	$ax=$EWB_b;
	push @as,$ax;
	$ax=".";
	push @as,$ax;
	$ax=$EWB_c;
	push @as,$ax;
	$ax=".";
	push @as,$ax;
	$ax=$EWB_d;
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
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	print $ax;
	
area101:
  $g=$g+1;
  goto area100;
area102: ;
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
};

#EasyWeB ver. 3.0alfa "simasm"
#by Enrico Betti e Paride Dominici


sub tab_islock($)
{ my $name=$_[0];
  my $l; my $r=0;
  foreach $l(@EWlocked)
  { if ($l eq $name) {$r=1;}
  } return $r;
}
