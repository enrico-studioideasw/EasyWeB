# Prove di contratto EWB

Questi programmi sono le prove usate durante l'allineamento tra
`sintassiTradotta.txt`, compilatore e VM.

Risultati testuali attesi:

- `compiler_smoke.ewb`: `2`
- `control_smoke.ewb`: `012Y`
- `foreach_smoke.ewb`: `AB`
- `array_functions_smoke.ewb`: `A-BAxBMarioG`
- `composed_smoke.ewb`: `3`
- `hash_smoke.ewb`: i digest MD5 e SHA-256 di `""`, `"abc"` e
  `"caffè ☕"`, concatenati nell'ordine del file
- `sql_orderby_smoke.ewb`: `nome, (cognome > 2)`
- `dataset_id_smoke.ewb`: `||`
- `db_compile_smoke.ewb`, se eseguito con DB: `1` e `persone.id` uguale al
  risultato di `add(persone)`.
- `consecutive_in_smoke.ewb`, se eseguito con DB: `AA`; verifica che QBYID non
  alteri i metadati del context fra due cicli IN consecutivi.
- `goal_db_smoke.ewb`: `lucaanna`

`form_smoke`, `form_resume_smoke` e `refresh_smoke` servono a ispezionare
HTML, stato sospeso e JavaScript. `db_compile_smoke` verifica la traduzione
SQL e il contratto dell'id restituito da ADD; se eseguito apre e chiude
esplicitamente la transazione.
`target_ask_smoke` verifica via HTTP che EWBD inoltri e autentichi il POST e
che un target sospeso da ASK non resti nello stato `running`.
`upload_path_smoke` verifica il contratto multipart condiviso da CGI ed EWBD:
al rientro il campo contiene i byte, mentre `_file` espone nome originale,
MIME type, dimensione e percorso temporaneo leggibile fino allo STOP.
`gd_smoke.ewb` salva un RGBA trasparente, esegue fill e crop, salva e ricarica
un PNG; il primo file deve essere 4x3 trasparente e il risultato 2x2 con pixel
RGBA rosso opaco. Lo
stesso contenitore, essendo una stringa binaria senza risorse vive, attraversa
la serializzazione dello stack usata da ASK senza un formato parallelo.

`test_hash.cpp`, eseguito da `make test`, verifica gli stessi sei vettori in
modo automatico. I caratteri non ASCII sono passati come byte UTF-8 esatti.

`goal_smoke` richiede che il gruppo `family` sia già popolato; la variante
`goal_db_smoke` prepara e ripulisce autonomamente il gruppo `smoke`.
