sub ewb_exist($)
{  $cond="\$res=join(\"\",((";
   $tab=$_[0];
   my $count=0;
   my @tot=split(":",$tab);
   foreach $field(@tot)
   { if ($count>0)
     { $cond=$cond."\$EWB_".$field;
       if ($count<$#tot) {$cond=$cond.","; };
     }
     $count++;
  }
  $cond=$cond.")=tab_exist(\"$tot[0]\",\$EWB_";
  $cond=$cond.$tot[1].")));";
  eval($cond);
  return ($res ne "");
}
