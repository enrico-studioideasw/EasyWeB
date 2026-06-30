#*** e qui le complicazioni
sub identifier($)
{ my $code=$_[0];
  my $res="";
  my $rcd;($code,$rcd)=delperc($code);$res.=$rcd;;
  my ($temp,$name)=variable($code);

  #vedo se e' una variabile
  my $v;
  my $found="";
  foreach $v(@variables)
  { if ( ((split(":", $v))[0]) eq $name)
    { $found="var";
    }
  };

  #vedo se e' una funzione (non e' ammesso overloading di operatori).
  #dovrei considerare anche le predefinite.
  if ($found eq	"")
  { foreach $v(@functions)
    { if ( ((split(":", $v))[0]) eq $name)
      { $found="fun";
      }
    }
  }
  if ($found eq "var")  { my $r;
			  ($code,$r)=delperc(substr($code, length($name)));
			  $res=$res.$r."\$ax=\$EWB_$name;$T";


  			};
  if ($found eq "fun")  { ($code,$res)=funzione($code);
                        };

  if ($found eq "")
  {
#vedo se e' una predefinita.. a questo punto ne e' permesso l' overloading.
     my $oldc=$code;
     #$res=$res."\$ax=";
     my $rb;
     ($code,$rb)=predeff($code);
     $res.=$rb;
#     print "#db predef\n";
     if ($code eq $oldc)
     { errore "funzione o variabile sconosciuta.";
     }
  }
  return ($code,$res);
}
