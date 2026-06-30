<<common/parametri.p>>

#funzione che controlla il path della tabella e aggiunge eventuali barre
<<common/checkpath.p>>

#funzione per conversione di caratteri speciali.
<<common/ewconv.p>>

#ricerca di parametro singolo 
<<common/get_par.p>>

#funzioni predefinite
<<common/load.p>>
<<common/loaddir.p>>
<<common/save.p>>
<<common/getenvvar.p>>

#i lock su piu' tabelle funzionano anche con DBI e mysql
<<common/tab_islock.p>>
<<common/tab_wlock.p>>
<<common/tab_unlock.p>>

#questi solo per database su files
<<files/tab_del.p>>
<<files/tab_exist.p>>
<<files/tab_sort.p>>
<<files/tab_read.p>>
<<files/tab_save.p>>

<<common/tab_login.p>>
<<common/delchar.p>>
<<common/addchar.p>>
<<common/createform.p>>

#liste - conversione da e a ascii e funzioni
<<common/toasc.p>>
<<common/fromasc.p>>
<<common/stpop.p>>
<<common/getelem.p>>
<<common/setelem.p>>
<<common/split.p>>
<<common/join.p>>

##socket
<<common/sock/socket.p>>
<<common/sock/server.p>>
<<common/sock/accept.p>>
<<common/sock/sread.p>>

<<common/prolog/funtore.p>>
#questa torna il funtore e il num. di parametri.
<<common/prolog/parsepart.p>>
<<common/prolog/isvariable.p>>
<<common/prolog/varval.p>>
<<common/prolog/match.p>>
<<common/prolog/assert.p>>
<<common/prolog/solve.p>>
<<common/prolog/ewassert.p>>
<<common/prolog/ewretract.p>>

#funzioni di data
<<common/date.p>>
<<common/lessdate.p>>

#funzioni di tempo
<<common/time.p>>

#######################codice#utente#######################
#<EWB_usercode>
############nuove funzioni della versione 2.0 #############

<<common/down/ewb_exist.p>>
<<common/down/ewb_login.p>>
<<common/down/ewb_remove.p>>
<<files/down/ewb_add.p>>
<<common/down/ewb_update.p>>

sub nothingelse() {};


