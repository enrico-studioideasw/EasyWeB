Circa 20 anni fa ho scritto il linguaggio amethist.  
Rispondeva a una domanda: come faccio a programmare sul web in modo algoritmico, senza dover pensare a continui rimpalli dei dati tra i client e il server.  
Per quello che poteva fare ha funzionato, e risolveva il problema.  

Oggi mi stavo ponendo un altro problema: come faccio ad eseguire un programma su un cluster "coprendo" il parallelismo con costrutti semplici e facendo in modo che la programmazione sia comoda e immediata?  
Non sono l'unico ad averci pensato.. ci sono occam per il transputer, e tante altre soluzioni fino a cuda.. che permettono di risolvere una parte di questo problema.. Nessuna di queste soluzioni è però generale.. perchè il problema è troppo ampio.  

Allora ragioniamo in maniera diversa.. riduciamo il problema. Oggi il 95% delle applicazioni che scrivo sono a base web.  
E sono cose piuttosto complesse: c'e' un programma principale, delle librerie, un protocollo solitamente inventato per l'occasione, una serie di richieste ajax che permettono di aggiornare parti di una pagina in maniera autonoma, e il tutto usando tre livelli: il web server, l'http e javascript.  
Questi tre livelli se usati correttamente e con il giusto server permettono di ottenere il parallelismo, ma a costo di una programmazione molto complessa, dell'uso attento di risorse condivise come database e files, del lancio sempre "attento" di applicazioni server side e con limiti molto forti per quanto riguarda i dati, ad esempio legati alla sessione.. e questo causa serializzazioni.  
Posso avere una batteria di web servers che rispondono ai clienti dando l'impressione di un programma interattivo. Ma io non ho scritto un programma interattivo, io sono impazzito tra richieste http, ajax, cron, processi, database, lock di risorse, sessione e suo rilascio, librerie e chiamate..  

Voglio "coprire" questa complessità. Il programmatore deve pensare al problema. Il runtime deve pensare all'infrastruttura.  
Ho detto coprire, non nascondere, perchè dove è necessario deve sempre essere possibile "scavalcare" questo strato per poter fare tutto..  limitarsi a quello che può fare il linguaggio è la cosa peggiore per un programmatore, significa porsi dei limiti prima ancora di affrontare un problema.  
Purtroppo una volta era una cosa normale: variabili limitate a 256 bytes sui vecchi microsoft basic, funzioni limitate a 64k su visual basic, sessioni che serializzano le chiamate di un singolo cliente al server.. oggi abbiamo la potenza di calcolo sufficiente a far fare queste cose alla macchina e a preoccuparci di cosa vogliamo ottenere più che di come farlo.  

Si, siamo nell'era dell IA. I programmatori sono obsoleti, quindi perchè un nuovo linguaggio ?  
L'IA oggi è un ottimo aiuto per scrivere codice, ma non elimina la necessità di progettare un sistema complesso.  
E il linguaggio è comunque importante. Quindi costruiamo un nuovo modello, migliore, più pulito e comprensibile.  
Poi lo useremo noi o qualche nostro assistente, non importa.  
Se fa quello che deve renderà il codice più semplice da scrivere e più comprensibile, quindi piu facile da correggere, verificare, aggiornare.. sia per noi che per le macchine.  

Quindi, come faccio a fare un programma PER IL WEB che nasconda ma che permetta di usare parallelismo di un cluster, di avviare tasks, di fare l'interfaccia come la voglio, di essere cosi semplice da essere dato in mano a un neofita ?  
Beh, e qui ci sono parecchie ore di conversazione tra me e l'IA appunto che mi ha aiutato a riesumare il modello di amethist esayweb per "estenderlo" e farlo diventare qualcosa di più recente. Non più la risposta algoritmica all programmazione web ma una risposta semplice alla programmazione su cluster nell'epoca del web inteattivo.  

Beh, intanto vediamo cosa faceva all'inizio di questo millennio amethist.  
La "killing application" era qualcosa di questo tipo:  

table utenti=user, passwd, datiriservati;  
b="entra";  
ask user, passwd:password, b:submit;  
if (login(utenti))  
{ show "Accesso valido: i tuoi dati riservati sono ".datiriservati."\n";  
};  
Scrivevo quella volta:  
In pratica e' il meccanismo con cui una sola pagina ewb e' in realta' un server che risponde a parecchie richieste http, anche contemporane, e per ciascun utente mantiene una sessione dimodoche' le richieste seguano un ordine logico analogo a quello di un programma con un singolo flusso di esecuzione.  
E' comunque da ricordare che il risultato non e' in realta' un programma a singolo flusso di esecuzione, anche quando viene usato da un singolo utente. Il linguaggio quindi prevede meccanismi impliciti per gestire la concorrenza di tabelle, e qualcosa di analogo va fatto per la gestione ad esempio di files.  
Il meccanismo viene implementato traducendo un programma ewb in un codice perl piuttosto complesso, con piu' punti di entrata, ciascuno in corrispondenza di una ask, ed un' area iniziale di recupero variabili e salto.  
ad ogni lancio della cgi in perl, si controlla una variabile del form: Questa variabile contiene come informazione la posizione a cui saltare per riprendere l' esecuzione.  
Lo stesso form contiene anche come nascoste tutte le variabili gia' esistenti al momento della ask.  
Questo significa che tutte le variabili dell' esecuzione corrente vengono ripristinate al ritorno dalla ask, e si puo' riprendere dalla riga successiva, indipendentemente da quanto tempo sia trascorso dall' operazione precedente, mentre in linguaggi come asp la sessione scade dopo un tempo limitato.  
I cicli vengono tutti sostituiti con istruzioni di salto condizionato per non avere problemi con lo stack al ritorno dalla ask. Il codice compilato e' costruito con una complessa struttura a stack in modo da poter annidare ask e cicli.  


Vi risparmio il linguaggio, le versioni, il passaggio a una VM quando divenne troppo lento.. E' preistoria.  

Torniamo al presente.  
Togliamo "amethist" (tanto il linguaggio sarà sobrio lo stesso..) e teniamoci easyweb, anzi solamente "ewb" come nome per il prompt, che va anche bene per "Enrico Betti Web" ;)..  
EWB non vuole essere un linguaggio migliore di C, PHP o Java. Vuole rendere più semplice scrivere applicazioni distribuite basate sul web senza costringere il programmatore a ragionare continuamente in termini di HTTP, AJAX, processi, lock e sessioni.  

I trucchi per farlo sono:  
dataset -> i database sono nascosti dentro una struttura dati convivisa, e sono gruppi di variabili attraversabili con cicli.  
tasks   -> La VM ewb può eseguire tasks asincroni con um meccanismo simile al CRON  
targets -> la pagina può essere sudivisa in sezioni e ciascuna di queste lavora in autonomia per il proprio aggiornamento.  
ask     -> Come nelle vecchie versioni la comunicazione attraverso forms viene serializzata.  
eventi  -> gli eventi di mouse e tastiera possono essere agganciati a targets.  

La comunicazione la fa il compilatore. Il programma si semplifica il più possibile.  


Stato attuale:  
- cluster per esecuzione parallela -> pronti 2 servers su tre, tests in corso  
- supporto CGI apache  -> pronto  
- web server dedicato per massimizzare le prestazioni -> pronto  
- VM che esegue codice .evm testuale o pseudo-assembler -> pronta ma richiederà qualche estensione  
- Compilatore ewb -> sviluppo parziale: struttura e primitive principali presenti, non ancora completo né pronto per uso generale  
- Supporto ewIA per programmazione assistita -> pronto  
- librerie di funzioni predefinite  -> ancora mancanti  
- motore di inferenza prolog basato sui dataset -> ancora mancante  

Il repository documenta anche checkpoint intermedi intenzionali: il codice può
compilare pur contenendo sottosistemi ancora incompleti. Questi checkpoint
servono a conservare e condividere la crescita del progetto; non equivalgono a
release stabili o complete.
