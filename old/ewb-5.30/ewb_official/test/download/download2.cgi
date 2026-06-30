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

my $ax,$bx,$cx,$dx,@as;
$ax="Location: frecce136.psa\n\n";
	print $ax;
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
};

#Amethist easyweb v. 5.27(pre 6)  
#Enrico Betti e Paride Dominici


