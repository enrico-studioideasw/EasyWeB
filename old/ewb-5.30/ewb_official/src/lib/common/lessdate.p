sub EWlessdate($$)
{ my ($d1,$d2)=@_;
  my ($day1,$mon1,$yea1)=split(" ",$d1);
  my ($day2,$mon2,$yea2)=split(" ",$d2);
  my $num1=$day1+($mon1*31)+($yea1*31*12);
  my $num2=$day2+($mon2*31)+($yea2*31*12);
  return ($num1<$num2);
}
