sub EWsocket($$)
{ my $remote= IO::Socket::INET->new( proto => "tcp",
                                     PeerAddr => $_[0],
                                     PeerPort => $_[1]);
  if ($remote) { $remote->autoflush(1); };
  return $remote;
}
