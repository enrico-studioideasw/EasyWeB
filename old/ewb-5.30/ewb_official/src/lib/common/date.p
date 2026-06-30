sub EWdate()
{ my @ti=localtime(time);
  if ($ti[3] < 10)
  { $ti[3] = '0'.$ti[3];
  };
  $ti[4] = $ti[4] + 1;
  if ($ti[4] < 10)
  { $ti[4] = '0'.$ti[4];
  };
  return $ti[3]." ".$ti[4]." ".($ti[5]+1900);
}
