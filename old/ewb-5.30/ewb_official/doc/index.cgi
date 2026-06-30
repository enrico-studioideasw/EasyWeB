#!/usr/bin/perl
print "Content-type: text/html\n\n";
#indice alla documentazione di easyweb, con supporto multilingua.
#dalla 5.0 la stessa documentazione potrà essere utilizzata con lingue diverse.

my $lingua;
my $default="Italiano";
$lingua="";
my  $pagina="";

#qui leggo da query string lingua e pagina
my  $k; $k=$ENV{QUERY_STRING};

($lingua,$pagina)=split("&", $k);
if (index($pagina, "/")>=0) {print "<html><body>Pagina non disponibile </body></html>\n\n"; exit;};
if ($lingua eq "") { $lingua=$default; };
if ($pagina eq "") { $pagina="indice.html"; };
printf "<meta Lingua=".$lingua." pagina=".$pagina.">\n";
#qui mostro i flag di lingua.
print "<div align=right>";
print "&nbsp;<a href=\"./index.cgi?Italiano&".$pagina."\"><img src=it.gif border=0></a>";
print "&nbsp;<a href=\"./index.cgi?English&".$pagina."\"><img src=en.gif border=0></a>";
print "</div>";


my  $a, $b;
#a=load pagina;
open iif, "< ".$pagina;
$a=join("", <iif>);
close iif;

#qui inserisco la versione corrente
my $ver=qx{ewb -v};
$a=join($ver, split("<VER>", $a));
#qui la lingua corrente, se necessaria.
$a=join($lingua, split("<LAN>", $a));


if ($lingua eq "Italiano")
{ #mostro tutte le parti di pagina contenenti un riferimento italiano
  #sostituisco i links
  #spezzo e tolgo <eng></eng>
  #spezzo e tolgo <frn></frn>
  my $i, $ok, $k;
  $ok=1; $b="";
  for ($k=0; $k<length($a); $k++)
  {
    if (substr($a, $k, 5) eq "<eng>") {$ok=0};
    if (substr($a, $k, 6) eq "</eng>") {$ok=1};
    if (substr($a, $k, 5) eq "<frn>") {$ok=0};
    if (substr($a, $k, 6) eq "</frn>") {$ok=1};
    if ($ok == 1) { printf substr($a, $k, 1); };
  };
};
if ($lingua eq "English")
{ #mostro tutte le parti di pagina contenenti un riferimento italiano
  #sostituisco i links
  #spezzo e tolgo <eng></eng>
  #spezzo e tolgo <frn></frn>
  my $i, $ok, $k;
  $ok=1; $b="";
  for ($k=0; $k<length($a); $k++)
  {
    if (substr($a, $k, 5) eq "<ita>") {$ok=0};
    if (substr($a, $k, 6) eq "</ita>") {$ok=1};
    if (substr($a, $k, 5) eq "<frn>") {$ok=0};
    if (substr($a, $k, 6) eq "</frn>") {$ok=1};
    if ($ok == 1) { printf substr($a, $k, 1); };
  };
};
