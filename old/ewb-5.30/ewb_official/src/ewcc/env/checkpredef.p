#definisco una funzione dim, e modifico la sintassi, in modo da
#costringere alla dichiarazione di variabili esplicita.
#la creazione di una tabella corrisponde alla dichiarazione
#di variabili se queste non esistono gia' in ambiente
#dichiarate esplicitamente.
#e' un overload di variabili se queste sono state dichiarate
#da un altra tabella.
sub checkpredef($)
{ #Controlla che la stringa passata non sia una predefinita.
  my @names=("if","for","while","do","sub","print","eprint","input","int","abs","cos",
               "sin","hex","sqr","random","mid","index","change","len","uc","asc",
               "char", "load", "save", "push", "pop", "getelem", "setelem", "numel", "return",
               "socket", "server", "accept", "sread", "swrite", "close",
               "lock", "unlock", "add", "exist", "login", "remove", "in", "inall", "sort",
               "ask","show","askto","form","showform", "assert", "retreat", "goal", "end", "goto",
               "format", "template" , "split", "join", "loaddir", "date", "time", "lessdate",
               "exec", "crypt", "getenvvar", "getremoteaddress", "getquerystring",
               "update", "del", "file", "loadtab", "savetab");

  my $a=$_[0];
  my $name;
  foreach $name(@names)
  { if ($name eq $a)
    { errore "Dichiarazione di variabile:nome associato a un token di linguaggio.";
    };
  };
};
