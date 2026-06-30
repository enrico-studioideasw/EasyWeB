sub assert($$)
{ my ($rule,$ruleset)=@_;
#un controllo veloce: una regola e' composta da part. o da part:=part[,part] . o !
  my $r=$rule;
  my $as=1;
  $r=substr($r,length(parsepart($r)));
  if ((substr($r,0,1) ne ".")&&(substr($r,0,1) ne "!"))
  { if (substr($r,0,2) ne ":=")
    { $as=0;}
    else
    { $r=substr($r,2);
      while (parsepart($r) ne "")
      { $r=substr($r,length(parsepart($r)));
        if (substr($r,0,1) eq ",") {$r=substr($r,1);};
      }
    }
  }
  if ((substr($r,0,1) ne ".") && (substr($r,0,1) ne "!"))
  { $as=0;};
  if ($as)
  { $ruleset=$ruleset.$rule."\n";
    $_[1]=$ruleset;
  } return $as;
}
