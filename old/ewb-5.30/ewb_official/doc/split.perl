#!/usr/bin/perl

open iif, "< easydummy.html";

$a = join("\n", <iif>);

close iif;

$i=0;
my $hd=((split("<part>", $a))[0]);

my @pp=split("<part>", $a);
foreach $part(split("<part>", $a))
{

print "pagina $i\n";

  open oof, "> ./page$i.html";
  print oof "$hd<br>
  <div align=center>";

  my $k;
  for ($k=1; $k<=$#pp; $k++)
  { print oof "<a href=index.cgi?<LAN>&page$k.html>$k </a>";
  }
  print oof "<br>";

  if ($i>1) { print oof "<a href=index.cgi?<LAN>&page".($i-1).".html>Prev</a>"; };

  if ($i<$#pp) { print oof "
              <a href=index.cgi?<LAN>&page".($i+1).".html>Succ</a>";
  }
  if ($i>0) { print oof "<br></div>$part
              </body></html>";
            }
  close oof;

  $i++;
}
  printf "Lavoro completato.\n";


