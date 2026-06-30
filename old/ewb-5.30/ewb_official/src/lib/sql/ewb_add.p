sub ewb_add($)
{  tab_create($_[0]);
   my @t=split(":", $_[0]);
   my $cond="my \$sql=\"INSERT INTO $t[0] (";
   my $i; 
   for ($i=1; $i<=$#t; $i++)
   { $cond=$cond.$t[$i].",";
   };
   $cond=substr($cond, 0, length($cond)-1);
   $cond=$cond.") VALUES (\";\n";
   my $tab=$_[0];
   my $count=0;
   my @tot=split(":",$tab);
   tab_wlock($tab);
   foreach $field(@tot)
   { if ($count>0)
     { $cond=$cond."\$sql.=\"'\".delchar(\$EWB_".$field.").\"'\";\n";
       if ($count<$#tot) {$cond=$cond."\$sql.=\",\";\n"; };
     }
     $count++;
  }
  $cond.="\$sql.=\")\";\n
  sqlrun(\$sql); ";
  eval($cond);
  tab_unlock($tab);
  return ($res ne "");
};
