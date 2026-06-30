#parsing di parametri aggiuntivi.
#-c        converte in formato dos 
#-v        versione
#-g        codici di debug (sorgente ewb)
#-text     programma per modo testo, non per web
#-dbf      tabelle su files
#-dbfv     tabelle su files .txt
#-dbmysql  tabelle mysql
my $headlib="dbfiles.lib";
my $bodylib;
my $addpar;
my $debugmode=0;
my $webmode=1;
my $convert2unix=1; 
do
{ $addpar=0;
  my $i;

  if ($ARGV[0] eq "-v")
  { print $version."\n"; exit(0);
  };
  if ($ARGV[0] eq "-g")
  { $debugmode=1;
    $addpar=1;
    for ($i=0; $ARGV[$i] ne ""; $i++)
    { $ARGV[$i]=$ARGV[$i+1];
    };
  };
  if ($ARGV[0] eq "-l")
  { $addpar=1;
    for ($i=0; $ARGV[$i] ne ""; $i++)
    { $ARGV[$i]=$ARGV[$i+1];
    };
    $webmode=0;
  };
  if ($ARGV[0] eq "-nw")
  { $addpar=1;
    for ($i=0; $ARGV[$i] ne ""; $i++)
    { $ARGV[$i]=$ARGV[$i+1];
    };
    $webmode=0;
  };
  if ($ARGV[0] eq "-dbfv")
  { $addpar=1;
    for ($i=0; $ARGV[$i] ne ""; $i++)
    { $ARGV[$i]=$ARGV[$i+1];
    };
    $headlib="dbfv.lib";
  };
  if ($ARGV[0] eq "-dbf")
  { $addpar=1;
    for ($i=0; $ARGV[$i] ne ""; $i++)
    { $ARGV[$i]=$ARGV[$i+1];
    };
    $headlib="dbfiles.lib";
  };
  if ($ARGV[0] eq "-dbmysql")
  { $addpar=1;
    for ($i=0; $ARGV[$i] ne ""; $i++)
    { $ARGV[$i]=$ARGV[$i+1];
    };
    $headlib="dbmysql.lib";
  };
  if ($ARGV[0] eq "-c")
  { $addpar=1;
    for ($i=0; $ARGV[$i] ne ""; $i++)
    { $ARGV[$i]=$ARGV[$i+1];
    };
    $convert2unix = 0; 
  };
} while ($addpar > 0);
