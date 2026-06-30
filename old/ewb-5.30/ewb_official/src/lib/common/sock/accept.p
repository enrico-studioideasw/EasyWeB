sub EWaccept($)
{ my $client= ($_[0])->accept();
  $client->autoflush(1);
  return $client;
}
