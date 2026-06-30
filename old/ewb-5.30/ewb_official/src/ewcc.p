#!/usr/bin/perl
use strict;
my $dbg=0;  #debug flag - a 1 per trovare errori nel compilatore.

my $version="Amethyst easyweb v. 5.30(pre 6)  
Enrico Betti e Paride Dominici
";
#5.30  corretti alcuni errori in array e gt 
#5.27  
#      * Corretto errore nella funzione pop(). 
#      Con valori vuoti nello stack faceva un gran casino.
#      (vedi errore_pop.ewb)
#      * Corretto errore nella funzione date(). Ora la data viene data sempre 
#      nello stesso formato con 2+2+4 cifre intramezzate da spazi. 
#      Prima con l'ordinamento dava dei problemi. (2 5 2005 => 02 05 2005)
#5.26  Aggiunta conversione automatica in formato unix per facilitare la
#      compilazione su windows.
#5.23  aggiunto table editor su ewb editor
#5.22  corretto un'errore sul passaggio del carattere + in textarea.
#5.21  modificate le procedure di installazione - pacchetti redhat/fedora.
#5.20: inserito amethyst web editor e l'installazione per 
#      windows in C scritta da Paride.
#5.18: Sono aggiunte le liste multiselezione options
#5.17: I campi check sono gestiti da format orizz e verticale 
#      e li ho aggiunti su dbsql
#5.16: Aggiunto il tipo di campo checks . I checks sono checkbox a scelta 
#      multipla. 
#5.15: modifiche alle ask per un maggiore controllo su nomi e tipi di campi. Per ora 
#      si comportava come se se fosse una funzione esterna al linguaggio
#5.14: modifiche alla delperc. Scartata per ora 
#5.13: i nomi di variabili non accettavano numeri al momento dell'uso 
#5.12: correnzione all'upload dei files perche'tagliava l'ultimo byte.
#5.11: correzione alle installazioni per windows
#5.08: modifica al parse dei token elementari
#      modifica alla funzione save
#5.09: modifica al parse di assegnamento

#versione 5: finalmente ho aggiunto il supporto mysql, l'istruzione 
#foreach e la gestione degli array con le quadre, anche per il multidimensionale!!
#ho quasi finito, ma la cosa e' piuttosto complicata. 
#ho riscritto la sintassi delle espressioni, e quindi non aspettatevi che tutto 
#funzioni bene. 
#forse c'e' qualche delperc in piu' quindi a[5] e a [5] e' accettato come la stessa
#espressione, ignorando gli spazi. contentatevi.

#sostituisco $code=delperc($code)
#con         my $rcd;($code,$rcd)=delperc($code);$res.=$rcd

#il modulo di gestione della pura compilazione.
#va modificato per compilare le espressioni anziche' tradurle in un linguaggio
#sintatticamente equivalente.
#il linker che aggiunge le funzioni predefinite sar�sviluppato a parte.
#per ora uso le stesse di ewb2 senza modifiche.
#Ho una variabile ax , accumulatore dove vanno le info. temporanee.
#Modifica dalla versione predecente:
#ogni token di linguaggio torna una lista di due valori: il codice ancora da gestire
# e il codice da lui letto e tradotto. Premette di riordinere il codice in uscita
#in qualunque momento
my $T="\n\t";

<<ewcc/parametri.p>>

#______________________________________________________________________________________
#ambiente
my @variables; my @tables, my @functions;
my $num_labels=0;
$variables[0]="template:v";
$variables[1]="format:v";
$variables[2]="tabpath:v";
$variables[3]="sqldbase:v";
$variables[4]="sqlserver:v";
$variables[5]="sqluser:v";
$variables[6]="sqlpassword:v";

my $num_calls=100;

#vecchie copie di ambiente: ricordo che una lista perl puo' contenere liste.
my @stack;

#@variables contiene nome:decltype
#                         decltype=v (var), t(tab)
#@tables contiene nome:campo:campo ...
#@functions contiene nome:arity
#calls contiene il numero di punti di salto a cui tornare per una return.

sub inblock()
{ push @stack, (join("\n", @variables));
  push @stack, (join("\n", @tables));
  push @stack, (join("\n", @functions));
}
sub outblock()
{ @functions = split("\n", pop @stack);
  @tables    = split("\n", pop @stack);
  @variables = split("\n", pop @stack);
}

<<ewcc/main.p>>

#qui parsers per la struttura di linguaggio. 

<<ewcc/parser/core.p>>

<<ewcc/parser/if.p>>

<<ewcc/parser/foreach.p>>

<<ewcc/parser/for.p>>

<<ewcc/parser/while.p>>

#qui gestione nella sintassi di espressioni  
<<ewcc/exp/assign.p>>

<<ewcc/exp/calcexp.p>>

<<ewcc/exp/inexp.p>>

<<ewcc/exp/cfr.p>>

<<ewcc/exp/eexp.p>>

<<ewcc/exp/term_e.p>>

<<ewcc/exp/term_t.p>>

<<ewcc/exp/term_f.p>>

<<ewcc/exp/term_b.p>>

<<ewcc/exp/term_k.p>>

<<ewcc/exp/term_c.p>>
#tokens...
<<ewcc/tokens/variable.p>>

<<ewcc/tokens/numero.p>>

<<ewcc/tokens/stringa.p>>

#___________________________________________________________________
#funzioni che settano  l' ambiente

<<ewcc/env/checkpredef.p>>

<<ewcc/env/tabdecl.p>>

<<ewcc/env/vardecl.p>>

<<ewcc/env/fundecl.p>>

#___________________________________________________________________
#token che usano l' ambiente
<<ewcc/envtokens/tabella.p>>

<<ewcc/envtokens/identifier.p>>

<<ewcc/envtokens/funzione.p>>

#___________________________________________________________________
#funzioni predefinite di linguaggio
<<ewcc/envtokens/predef.p>>

<<ewcc/predef/print.p>>

<<ewcc/predef/eprint.p>>

<<ewcc/predef/getenvvar.p>>

<<ewcc/predef/getremoteaddress.p>>

<<ewcc/predef/getquerystring.p>>

<<ewcc/predef/input.p>>

<<ewcc/predef/num/int.p>>

<<ewcc/predef/num/abs.p>>

<<ewcc/predef/num/cos.p>>

<<ewcc/predef/num/sin.p>>

<<ewcc/predef/num/hex.p>>

<<ewcc/predef/num/sqr.p>>

<<ewcc/predef/num/random.p>>

<<ewcc/predef/date.p>>

<<ewcc/predef/time.p>>

<<ewcc/predef/lessdate.p>>

<<ewcc/predef/str/crypt.p>>

<<ewcc/predef/return.p>>

<<ewcc/predef/str/mid.p>>

<<ewcc/predef/str/index.p>>

<<ewcc/predef/str/join.p>>

<<ewcc/predef/str/split.p>>

<<ewcc/predef/str/change.p>>

<<ewcc/predef/exec.p>>

<<ewcc/predef/str/len.p>>

<<ewcc/predef/str/uc.p>>

<<ewcc/predef/asc.p>>

<<ewcc/predef/end.p>>

<<ewcc/predef/char.p>>

<<ewcc/predef/files/load.p>>

<<ewcc/predef/files/loaddir.p>>

<<ewcc/predef/files/save.p>>

<<ewcc/predef/array/numel.p>>

<<ewcc/predef/array/push.p>>

<<ewcc/predef/array/setelem.p>>

<<ewcc/predef/array/getelem.p>>

<<ewcc/predef/array/pop.p>>

#___________________________________________________________________
#Gestione semplificata di socket.
#per ora solo tcp.
<<ewcc/predef/sockets.p>>

<<ewcc/predef/sockets/socket.p>>

<<ewcc/predef/sockets/swrite.p>>

<<ewcc/predef/sockets/accept.p>>

<<ewcc/predef/sockets/server.p>>

<<ewcc/predef/sockets/close.p>>

<<ewcc/predef/sockets/sread.p>>

#___________________________________________________________________
#tabelle di database
#lock, unlock, add, remove, exist, login, in
<<ewcc/predef/database.p>>

<<ewcc/predef/db/savetab.p>>

<<ewcc/predef/db/loadtab.p>>

<<ewcc/predef/db/lock.p>>

<<ewcc/predef/db/unlock.p>>

<<ewcc/predef/db/sort.p>>

<<ewcc/predef/db/add.p>>

<<ewcc/predef/db/update.p>>

<<ewcc/predef/db/exist.p>>

<<ewcc/predef/db/login.p>>

<<ewcc/predef/db/remove.p>>

<<ewcc/predef/db/in.p>>

<<ewcc/predef/db/inall.p>>

<<ewcc/predef/db/del.p>>

<<ewcc/predef/db/checktable.p>>

#___________________________________________________________________
#area di comunicazione con il webbo
<<ewcc/predef/web.p>>

<<ewcc/predef/web/show.p>>

<<ewcc/predef/web/form.p>>

<<ewcc/predef/web/field.p>>

<<ewcc/predef/web/showform.p>>

#questa serve a far saltare il codice di ritorno al punto giusto.
<<ewcc/predef/web/addlabel.p>>

<<ewcc/predef/web/ask.p>>

<<ewcc/predef/web/askto.p>>

#----- http url parser.
<<ewcc/predef/web/geturl.p>>

#___________________________________________________________________
#motore inferenziale poggiato su tabelle
# funzionante dalla versione 2.0, credo.

<<ewcc/predef/prolog.p>>

<<ewcc/predef/prolog/assert.p>>

<<ewcc/predef/prolog/retreat.p>>

<<ewcc/predef/prolog/goal.p>>

#___________________________________________________________________
