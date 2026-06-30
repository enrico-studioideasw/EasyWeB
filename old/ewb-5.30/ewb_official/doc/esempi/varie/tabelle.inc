'database per il sistema cesi. 
'questo va incluso in tutte le funzioni easyweb della gestione del sistema. 

table struttura=nomes,tels,citta,indirizzo;
table referenti=nomecogn,datainser,telr,nomeassoc,ftpclient;
table rete=nomer,nomecogn;
table virtualhost=vnome;
table tipiintervento=tipo;
table risorsaweb=url,nomecogn,chiaveprog,user,user1,vnome;
table accessi=user,password;
table strumenti=nomestrumento;
table host=chiaveprog,ip,hostname,nomehost,sistop,proprietario,hhost,tecnologia,velocita;
table lavoraper=lvp; '(nomecogn,nomes)
table sitoweb=stw; '(nomes,url)
table indirizzi=idr; '(citta,indirizzo)
table areaip=aip,rete,netmask,broadcast,dimensione; '(nomer,chiaveprog)
table personale=nomepers,telp, nomecogn, user; 
table interventi=intv,tipo,nomecogn,data2,ora2,chiaveprog,memo,esito; '(nomepers,data1,ora1)
table tecnologie=nome;
template=load "open.html";

