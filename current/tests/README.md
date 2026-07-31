# Prove di contratto EWB

Questi programmi sono le prove usate durante l'allineamento tra
`sintassiTradotta.txt`, compilatore e VM.

Risultati testuali attesi:

- `compiler_smoke.ewb`: `2`
- `control_smoke.ewb`: `012Y`
- `foreach_smoke.ewb`: `AB`
- `array_functions_smoke.ewb`: `A-BAxBMarioG`
- `composed_smoke.ewb`: `3`
- `sql_orderby_smoke.ewb`: `nome, (cognome > 2)`
- `dataset_id_smoke.ewb`: `||`
- `goal_db_smoke.ewb`: `lucaanna`

`form_smoke`, `form_resume_smoke` e `refresh_smoke` servono a ispezionare
HTML, stato sospeso e JavaScript. `db_compile_smoke` verifica la traduzione
SQL; se eseguito apre e chiude esplicitamente la transazione.

`goal_smoke` richiede che il gruppo `family` sia già popolato; la variante
`goal_db_smoke` prepara e ripulisce autonomamente il gruppo `smoke`.
