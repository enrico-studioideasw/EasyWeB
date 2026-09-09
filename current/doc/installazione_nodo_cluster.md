# Installazione di un nodo EWB secondario

Lo script [`../tools/install-ewb-node`](../tools/install-ewb-node) replica su
un nodo Debian l'ambiente applicativo del nodo EWB sorgente. Installa le
dipendenze, monta via NFS il document root e lo stato condiviso, copia runtime
e configurazioni locali, attiva `ewbd` e Caddy e infine collauda le pagine
principali.

L'installer è pensato per l'attuale cluster di Civolle, non per una macchina
Debian generica. I valori predefiniti sono:

- sorgente NFS: `192.168.2.51` (`cluster1`);
- rete dei nodi: `192.168.2.0/24`;
- chiave operativa: `/root/.ssh/cluster_backup_ed25519`;
- codice condiviso: `/home/noc/EWB2026/current/www`;
- stato condiviso: `/home/noc/EWB2026/shared-state`.

## Prima dell'esecuzione

Lo script deve essere eseguito come `root` sul nodo sorgente. Il destinatario
deve essere un nodo secondario Debian già raggiungibile come `root` con la
chiave indicata e deve avere l'utente `noc` con UID e GID uguali a quelli del
sorgente.

Dopo la reinstallazione di un server, verificare fuori dallo script la nuova
impronta SSH e aggiornare consapevolmente `known_hosts`. L'installer usa
`StrictHostKeyChecking=yes` e non accetta automaticamente chiavi nuove.

Controllare inoltre che MariaDB consenta l'accesso dell'utente applicativo
dalla rete dei nodi e che cluster1 esporti via NFS i due percorsi richiesti.

## Uso

Dal checkout del repository su cluster1:

```sh
current/tools/install-ewb-node 192.168.2.53
```

Il nodo sorgente viene sempre rifiutato come destinazione. Prima di modificare
il nodo remoto lo script salva in `/root/ewb-node-before-install-*.tar.gz` le
configurazioni e i binari EWB preesistenti. L'operazione è ripetibile: mount,
link e unità systemd vengono ricondotti alla configurazione attesa.

I parametri locali possono essere sostituiti tramite `EWB_SOURCE_HOST`,
`EWB_SOURCE_NETWORK`, `EWB_SSH_KEY`, `EWB_WWW` ed `EWB_SHARED`. Questa
possibilità non elimina gli altri vincoli dell'installazione, in particolare i
percorsi usati dall'unità systemd e i test HTTP specifici di `civolle.it`.

## Stato del collaudo

La versione originaria è stata eseguita ripetutamente con successo su cluster3
il 9 settembre 2026. Pagina statica, Famtree e Forum-one rispondevano HTTP 200;
una sessione PHP creata su cluster1 è stata riletta su cluster2 e cluster3 con
token CSRF invariato. Le correzioni successive conservano la stessa procedura,
ma richiedono un nuovo collaudo operativo alla prossima installazione reale.
