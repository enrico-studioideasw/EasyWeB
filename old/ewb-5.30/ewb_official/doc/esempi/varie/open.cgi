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

sub EWload($)
{ open iif,"<".$_[0];
  my $a=join("",<iif>);
  close iif;
  return $a;
}

my $ax,$bx,$cx,$dx,@as;
$ax="open.html";
	$ax=EWload($ax);
	$EWB_template=$ax;
	$ax="
<br><a href=richiediintervento.cgi>Richiedi un intervento al CESI</a>
<br><a href=intervento.cgi>Segnala il completamento di un intervento</a>
<br>
<br>Persone
<br><a href=aggiungipersonale.cgi>Aggiungi personale</a>
<br><a href=aggiungimail.cgi>Aggiungi un utente di posta</a>
<br><a href=rimuovimail.cgi>Togli un utente di posta</a>
<br><a href=accountweb.cgi>Account web</a>
<br>
<br>Rete
<br><a href=hosts.cgi>Gestione hosts</a>
<br><a href=links.cgi>Gestione links</a>
<br>
";
	$ax=print join(($ax), split("<>",$EWB_template));
	exit(0); 

sub jumper($)
{ my $lab=$_[0];
};

#Versione 3.11 pitbull
#Enrico Betti e Paride Dominici


