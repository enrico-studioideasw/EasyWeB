sub tab_save($$)
{ #esegue la scrittura di una tabella gia bloccata.
  my @t=split(":", $_[0]);
  my $sql="DELETE * from $t[0]";
  sqlrun($sql);
  my $i;
  #questo eventualmente crea la tabella con una create table sql.
  tab_create($_[0]);

  $sql="INSERT INTO $t[0] (";
  for ($i=1; $i<$#t; $i++)
  { $sql=$sql.$t[$i].",";
  };
  $sql=$sql.$t[$i].") VALUES (";
  my $l, $p, $a;
  foreach $l(split("\n", $_[1]))
  { $a=$sql;
    foreach $p(split("\n", $l))
    { $a=$a."'".$p."',";
    };
    $a=substr($a, 0, length($a)-1).")";
    runsql($a);
  };
};
