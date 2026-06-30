#*** qui vanno gestite le push su @varstack
sub funzione($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;
  {
    my $name;
    ($code, $name)=variable($code);

    #qui salvo l' indirizzo corrente....
    #devo ricordarmi come ho fatto nel resto del codice.
    my $nc=$num_calls;
    $num_calls=$num_calls+1;

    my $addr= "area$nc";

#print "addr: $addr\n";
    $res.="push \@varstack, $addr;$T";


#se ha delle parentesi in  testa le tolgo.. analogo alle predefinite.
#gestione parentesi 1/2

  my $hp=0;
  if (substr($code,0,1) eq "(")
  { my $t; ($code,$t)=delperc(substr($code,1));
    #$res=$res. "(";
    $hp=1;
  }

    #secondo la arity valuto i parametri.
    my $arity;
    my $v;
    foreach $v(@functions)
    { if ( ((split(":", $v))[0]) eq $name)
      { $arity=((split(":", $v))[1]);
      }
    }
    my $i;
#modifica  030502P inizio
    my $temp2;
    my $temp3="";
    my $temp="";
    $temp2="";
#print "\narity=$arity\n"; ###PAR
#modifica  030502P fine
    for($i=0; $i<$arity; $i++)
    { my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;

      my $temp;

      ($code,$temp)=calcexp($code);
#modifica  030502P inizio
#     $res.=$temp;
      $temp3=$temp;
#printf "\nArity: $arity\n"; print $res;  exit;
#      $res.="push \@varstack, \$ax;$T";
      $temp3.="push \@varstack, \$ax;$T";
      $temp2=$temp3.$temp2;
#modifica  030502P fine

      my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
      if ($i<$arity-1)
      { if (substr($code, 0, 1) ne ",")
        { errore "$name: manca un parametro.";
        }
        $code=substr($code, 1);
      }
    }
#modifica  030502P inizio
    $res.=$temp2;
#modifica  030502P fine

#seconda parte della gestione parentesi.
#gestione parentesi 2/2
    if($hp)
    { if (substr($code,0,1) ne ")")
      { errore "$name: manca ).";
      }
      my $t; ($code,$t)=delperc(substr($code,1));
    }
#chiamata vera e propria della funzione con un jump.
    $res.="goto JMPEWB$name;$T";
    $res.="\n$addr:$T";

#print "Ritorno: $addr\n";
  }
  return ($code,$res);
}
