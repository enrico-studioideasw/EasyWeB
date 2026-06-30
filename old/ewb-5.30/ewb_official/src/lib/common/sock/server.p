sub EWserver($$)
{ my $remote= IO::Socket::INET->new( Proto => "tcp",
                                     Localport => $_[0],
                                     Listen => 5,
                                     Reuse => 1);
  return $remote;
}

