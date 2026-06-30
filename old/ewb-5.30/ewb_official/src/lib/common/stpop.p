sub stpop($)
{  
   my $test = $_[0];
   my $pos=0;
   $test=substr($test, 0, length($test) - 1);
   $pos=rindex($test, ' ');
   if ($pos != -1)
   {
     $res=substr($test, $pos + 1);
   }else
   {
     $res=$test;
   }
   $_[0] = $test;
   my $len= length($_[0])-(length($res));
   $_[0]=substr($_[0], 0, $len);
   return fromasc($res);
}
