sub solve($$$)
{ my ($rule,$ruleset,$vars)=@_;
  my $cut=0;
  my $ook=0;my $l;
  foreach $l(split("\n",$ruleset))
  {
    {
      my $a=$vars;
      my $k=$rule;
      if ((match($rule,$l,$vars)))
      { $ook=1;
	#attenzione - passo da $rule a $l
        $rule=substr($l,length(parsepart($l)));
        if(substr($rule,0,2) eq ":=") { $rule=substr($rule,2); };
	if ($cut==1) {$ook=0;}
        if ((substr($rule,0,1) ne ".") && (substr($rule,0,1) ne "!") && ($cut==0))
        {  while ((substr($rule,0,1) ne ".") && (substr($rule,0,1) ne "!"))
           {
             if(substr($rule,0,1) eq ",") { $rule=substr($rule,1); };
             my $a=parsepart($rule);
             $rule=substr($rule,length($a));
             #il not e' registrato come ~.
             if (substr($a,0,1) eq "~")
             { $ook=$ook & (!solve(substr($a,1), $ruleset,$vars));
             } else
             { $ook=$ook & solve($a,$ruleset,$vars);
             };
           }
	}
        else
        { #se la regola e' un goal, sono in fondo: memorizzo $vars valorizzate.
          if (!$cut) {
	  #print "soluzione:".join(" - ",split("\n",$vars))."\n";
	  $rlist=$rlist.join("\t",split("\n",$vars))."\n";
           }
	  $ook=1;
          if (substr($rule,0,1) eq "!") {$cut=1;};
        };
      }
      $vars=$a;
      $rule=$k;
    }
  }
  if ($ook) { $_[2]=$vars; };
  return $ook;
}
