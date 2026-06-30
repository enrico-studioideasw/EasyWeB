# EWB2026 / easyweb

Workspace per la riscrittura di EasyWeb.

## Struttura

- `current/`: appunti e codice corrente EWB2026.
- `old/`: versioni storiche conservate come riferimento e materiale archeologico.

## Note

Il codice corrente e' ancora prototipale. La prima direzione tecnica e':

- VM testuale e debuggabile;
- primitive C-like con `std::string`;
- daemon `ewbd` con coordinator/responder prefork;
- stato applicativo stateless rispetto al server, da firmare/cifrare negli strati successivi.

