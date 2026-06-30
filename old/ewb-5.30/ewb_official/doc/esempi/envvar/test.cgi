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

sub EWgetenvvar($)
{
  my $r;
  $r= $ENV{$_[0]};
  return $r;
}

#2.10 fine
my $ax,$bx,$cx,$dx,@as;

my $EWB_a;
$ax="<BR>REMOTE_ADDR >";
	push @as,$ax;
	$ax=(EWgetenvvar("REMOTE_ADDR"));
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>QUERY_STRING >";
	push @as,$ax;
	$ax=(EWgetenvvar("QUERY_STRING"));
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<br>------------------<br><BR>REMOTE_ADDR
>";
	push @as,$ax;
	$ax="REMOTE_ADDR";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>REMOTE_HOST >";
	push @as,$ax;
	$ax="REMOTE_HOST";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>SERVER_NAME >";
	push @as,$ax;
	$ax="SERVER_NAME";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>SERVER_PORT >";
	push @as,$ax;
	$ax="SERVER_PORT";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>DOCUMENT_ROOT >";
	push @as,$ax;
	$ax="DOCUMENT_ROOT";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>HTTP_REFERER >";
	push @as,$ax;
	$ax="HTTP_REFERER";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>HTTP_USER_AGENT >";
	push @as,$ax;
	$ax="HTTP_USER_AGENT";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>PATH_INFO >";
	push @as,$ax;
	$ax="PATH_INFO";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>PATH_TRANSLATED >";
	push @as,$ax;
	$ax="PATH_TRANSLATED";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	push @as,$ax;
	$ax="<BR>QUERY_STRING >";
	push @as,$ax;
	$ax="QUERY_STRING";
	$ax=EWgetenvvar($ax);
	$bx=pop @as;
	$ax=$bx.$ax;
	$bx=pop @as;
	$ax=$bx.$ax;
	$EWB_a=$ax;
	$ax=$EWB_a;
	$ax=print join(($ax), split("<>",$EWB_template));
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
};

#Versione 3.13 beagle 
#Enrico Betti e Paride Dominici


