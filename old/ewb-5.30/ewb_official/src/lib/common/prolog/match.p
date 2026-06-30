sub match($$$)
{my ($clausea,$clauseb, $vars)=@_;
  my $a=funtore $clausea;
  my $az=parsepart $clausea;
  $clausea=substr($clausea, length($a));
  my $b=funtore $clauseb;
  my $bz=parsepart $clauseb;
  $clauseb=substr($clauseb, length($b));
  my $res="";

  if (!isvariable($a) && !isvariable($b))
  { #devono corrispondere i funtori e matchare gli argomenti.
    if ($a eq $b)
    { $res=1;
      if ( ($a ne $az)&&(substr($clauseb,0,1) eq "(")&&(substr($clausea,0,1) eq "(") )
      { $res=1;
        #pesco gli argomenti e ne richiedo il match..
        $clausea=",".substr($clausea,1);
        $clauseb=",".substr($clauseb,1);
        do
        { $clausea=substr($clausea,1);
          $clauseb=substr($clauseb,1);
	  if (($clausea ne ")") && ($clauseb ne ")"))
	  { if  (match($clausea,$clauseb,$vars)ne 1)
	    {$res=0;};
	  }
#qui tolgo la clausola di testa.
        $clausea=substr($clausea,length(parsepart($clausea)));
        $clauseb=substr($clauseb,length(parsepart($clauseb)));
        } while ((substr($clausea,0,1) eq ",") && (substr($clauseb,0,1) eq ","));
        $res=$res && match($clausea,$clauseb,$vars);
        if ((substr($clausea,0,1) ne ")" ) || (substr($clauseb,0,1) ne ")"))
        { $res=0;
        }
        if ($res eq 1)
        { $clausea=substr($clausea,0,1);
          $clauseb=substr($clauseb,0,1);
        }
      }
    }
  }
  elsif (isvariable($a) && isvariable($b))
  { if ((varval($a,$vars)eq "<Null>") && (varval($b,$vars) eq "<Null>"))
    { varassign $a,$b, $vars; $res=1;
    }
    elsif ((varval($a,$vars)eq "<Null>") && (varval($b,$vars) ne "<Null>"))
    { varassign($a,varval($b,$vars), $vars); $res=1;
    }
    elsif ((varval($a,$vars) ne "<Null>") && (varval($b,$vars) eq "<Null>"))
    { varassign($b, varval($a,$vars),$vars); $res=1;
    }
    else
    { $res=match(varval($a,$vars), varval($b,$vars), $vars);
    }

  }
  elsif (isvariable($a) && !isvariable($b))
  { if (varval($a,$vars) eq "<Null>")
    { varassign($a, $bz,$vars); $res=1;
    }
    else {
    $res=match(varval($a,$vars),$bz, $vars);
    }
  }
  elsif (!isvariable($a) && isvariable($b))
  { if (varval($b,$vars) eq "<Null>")
    { varassign($b, $az,$vars); $res=1;
    }
    else {$res=match($az,varval($b,$vars), $vars); }
  }
  #effetto collaterale. Cambio le variabili passate per valore.
  if ($res) { $_[2]=$vars; }
  return $res;
}
