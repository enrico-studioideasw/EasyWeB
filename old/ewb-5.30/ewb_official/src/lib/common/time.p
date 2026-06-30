sub EWtime()
{ my @ti=localtime(time);
  if ($ti[2] < 10)
  { $ti[2] = '0'.$ti[2];
  };
  if ($ti[1] < 10)
  { $ti[1] = '0'.$ti[1];
  };
  if ($ti[0] < 10)
  { $ti[0] = '0'.$ti[0];
  };
  return $ti[2].":".($ti[1]).":".($ti[0]);
}
