sub is_tabella($)
{ my ($code,$n)=delperc($_[0]);
  my $res=0;
  if (is_identifier($code))
  { my ($temp,$n)=variable($code);
    my $t;
    foreach $t(@tables)
    { my @p=split(":", $t);
      if (($res==0)&&(@p[0] eq $n))
      { $res=1;
      };
    }
  }
  return $res;
}
#***
sub tabella($)
{ my ($code,$n)=delperc($_[0]);
  my $res="\$ax=";
  ($code, $n)=variable($code);
  my $t;
  foreach $t(@tables)
  { my @p=split(":", $t);
    if (@p[0] eq $n)
    { $res=$res. "\"".$t."\";\n";
    }
  }
  return ($code,$res);
}
