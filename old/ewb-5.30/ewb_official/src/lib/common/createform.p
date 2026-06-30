sub CreateForm
{ #nomecampo, valore, tipo campo, nome a video
  my ($n,$v,$t,$vidname)=@_;
  if ($t eq "hidden")
  { #conversione di un campo hidden, analoga a push
    $v="HidDenFieldDaTa".toasc($v);
  }
  else
  { if (($t ne "textarea")&&($t ne "info"))
    { $v=join("%22", (split "\"", $v));
      $v=join("%09", (split "\t", $v));
      $v=join("%0A", (split "\n", $v));
      $v=join("%3C", (split "<", $v));
    };
  };
  my @f=split(":", $EWB_format);
  if ($f[0]<1) {$f[0]=12; };
  if ($f[1]<1) {$f[1]=5; };

  my $res;
  my $vn=$vidname;
  if (length($vidname) <2) { $vidname=$n; };
  $res="<table border=0><tr><td>$vidname</td><td>";

  if (($t eq "submit") && (length($vn)<2)) { $res="<table border=0><tr><td></td><td>"; };
  if ($t eq "file")
  { $res=$res."<input name=\"$n\" value=\"\" type=file>"; }
  if ($t eq "text")
  { $res=$res."<input name=\"$n\" value=\"$v\" size=".$f[0].">"; }
  if ($t eq "password")
  { $res=$res."<input name=\"$n\" type=password value=\"$v\" size=".$f[0].">"; }
  if ($t eq "hidden")
  { $res="<input name=\"$n\" type=hidden value=\"$v\">"; }
  if ($t eq "checkbox")
  { $res=$res."<input name=\"$n\" type=checkbox value=\"1\">"; }
  if ($t eq "info")
  { $res="<table border=0><tr><td><b> $v </b>"; }
  if ($t eq "textarea")
  { $res=$res."<textarea name=\"$n\" cols=".$f[0]." rows=".$f[1].">$v</textarea>"; }
  #tipi composti
  if ($t eq "option") { $res=$res."\n<select name=\"$n\">"; }
  if ($t eq "options") { $res=$res."\n<select name=\"$n\" multiple>"; }
  my @v=split(/;/,$v); my $p;

  if ($t eq "checks")
  { $res=$res."<input name=\"$n\" type=hidden value=\"\">\n";
  }

  my $sep; if ($f[2] eq "v") { $sep="<br>"; } else{ $sep=" &nbsp; "; };
  my $counter=0;
  foreach $p(@v)
  {
    if ($t eq "checks")
    { my $CV="";
      if (substr($p, 0, 1) eq  "*")
      { $p=substr($p, 1);
        $CV="checked";
      };
      $res=$res."<input name=\"EWcHECK_".$counter.$n."\" type=checkbox $CV value=\"$p\">$p $sep\n";
      $counter++;
    }
    if ($t eq "links")
    {  my ($vid,$dest)=split("->", $p);
       if ($dest eq "") { $dest=$vid; };
       $res=$res."<a href=\"$dest\">$vid</a>$sep"; }
    if ($t eq "radio")
    { if ( substr($p,0,1) eq "*")
      { $p=substr($p,1);
        $res=$res."<input type=radio name=\"$n\" value=\"$p\" checked>$p$sep";
      }
      else
      { $res=$res."<input type=radio name=\"$n\" value=\"$p\">$p$sep";
      };
    }
    if (($t eq "option") || ($t eq "options"))
    { if ( substr($p,0,1) eq "*")
      { $p=substr($p,1);
        $res=$res."<option value=\"$p\" selected>$p";
      }
      else
      { $res=$res."<option value=\"$p\">$p";
      };
     }
    if ($t eq "submit")
    { $res=$res."<input name=\"$n\" type=submit value=\"$p\">$sep"; }
  }
  if (($t eq "option")||($t eq "options")) { $res=$res."</select>"; }
  if ($t ne "hidden") { $res=$res."</td></tr></table>"; }
  return $res."\n";
}
