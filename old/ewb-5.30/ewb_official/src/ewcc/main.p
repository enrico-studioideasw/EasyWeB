#______________________________________________________________________________________

#*****************main*main*********************#
#uso ewcc filename outputfile
my $res;
#modifica 030502P Inizio
#if ($ARGV[1] eq "")
if ($ARGV[0] eq "")
#modifica 030502P Fine
{ printf stderr "$version
Uso ewb [parametri]filename [outfile]
	parametri:
  -c        converte in formato dos 
  -v        mostra la versione corrente. 
  -g        inserisce codici di debug
  -nw, -l		nessun motore di accesso web
  -dbmysql	usa database engine mysql
  -dbf      usa database su files
  -dbfv     usa database su files visibili (.txt)

Nota : Se outfile e' omesso viene creato da filename.ewb filename.cgi

"; exit 0;
};
#modifica 030502P Inizio
# Se manca il secondo parametro creo un file
# con lo stesso nome ma con estensione .cgi
if ($ARGV[1] eq "")
{
  my @a;

  @a=split(/\./ ,$ARGV[0]);
#  print "=>$#a-$a[0]_$a[1]-$ARGV[0]\n";
  $ARGV[1]= $a[0]."\.cgi";
#  print "=>$#a-$a[0]_$a[1]-$ARGV[1]\n";
#  exit(0);
};
#modifica 030502P Fine


my $code;
if (open iiif, "<".$ARGV[0])
{
         $code=join("", <iiif>);
         close iiif;
  open oof, ">".$ARGV[1];
} else
{ print "Il file $ARGV[0] non sembra esistere.\nErrore\n"; 
};
sub delperc($);
#*****************preproc***********************#
#qui lancio del preprocessore.
$code=ppmain($code);
#mi tengo la riga corrente in una variabile
my $curr_row=(split("\n", $code))[0];
my $num_row=1;

my $rr;
($code, $rr)=delperc($code);
#print $code; exit;

#print "##\t".$curr_row."\n";
($code,$res)=programma($code);
my $v=join("\n#", split("\n", $version));
$res.= "\n#$v\n";


#fine compilazione.. $res contiene il compilato.

#*****************linker************************#

if ($webmode!=0)
{ $res="
my \@parname;
my \@parvalue;
srand(time());
read_form_buffer();
#jumper area
if (get_par(\"EWjump\")>0)
{ jumper(get_par(\"EWjump\"));
};
".$res;
};
$res=lnmain($res,$headlib);

if ($webmode!=0)
{ $res="#!/usr/bin/perl
"."print \"Content-type: text/html\\n\\n\"; \n".$res;
};

print oof $res;
close oof;
# converto in unix se devo
# questa parte fa qualcosa di sensato solo in windows
# in *nix il file e' gia' in formato giusto per cui perdo 
# solo tempo
if ($convert2unix == 1)
{ 
  print "\nConverto il file nel formato UNIX\n";
  my $convertedline;
  my @convertedfile;
  #converto il file generato in formato unix 
  open iif, "<$ARGV[1]" ;
    @convertedfile = <iif>;
  close iif;
  
  open oof, ">$ARGV[1]" ;
  binmode oof;
    foreach $convertedline(@convertedfile)
    {
      printf oof "%s%c",join("", split("\n", $convertedline)), 10;
    };
  close oof;
}else{
  print "\nLascio il file invariato.\n";
};
#rendo eseguibile il compilato.
chmod 0755, $ARGV[1];
exit(0);
#*****************fine*main*********************#
