typedef struct
{ char * name;
  //int arity;
  char sql; 
} pref;

pref pred[100];
int is_predef(char* f)
{ for (i=0; pred[i]!=NULL; i++) 
  if (!strcmp(tolower(f),pred[i].name)
  { if (inside_sql && pred[i].sql==0) err("Unsupported in SQL"); 
    return true;
  };
  return false;
};
predef(char* name)
{ //Ho i parametri, qui gestisco la funzione. Se sono predefinite di solito vuol dire che serve un istruzione vm
  //Database
  if (!strcmp(name,"add"))     vp_add();
  else if (!strcmp(name,"exists"))   p_exists();
  else if (!strcmp(name,"lock"))     p_lock();
  else if (!strcmp(name,"unlock"))   p_unlock();
  //Input/output
  else if (!strcmp(name,"print"))    p_print();
  else if (!strcmp(name,"eprint"))   p_eprint();
  else if (!strcmp(name,"input"))    p_input();
  else if (!strcmp(name,"show"))     p_show();
  //Filesystem
  else if (!strcmp(name,"load"))     p_load();
  else if (!strcmp(name,"save"))     p_save();
  else if (!strcmp(name,"loaddir"))  p_loaddir();
  //Matematiche e stringhe
  else if (!strcmp(name,"int"))      p_int();
  else if (!strcmp(name,"abs"))      p_abs();
  else if (!strcmp(name,"cos"))      p_cos();
  else if (!strcmp(name,"sin"))      p_sin();
  else if (!strcmp(name,"hex"))      p_hex();
  else if (!strcmp(name,"sqr"))      p_sqr();
  else if (!strcmp(name,"asc"))      p_asc();
  else if (!strcmp(name,"char"))     p_char();
  else if (!strcmp(name,"mid"))      p_mid();
  else if (!strcmp(name,"len"))      p_len();
  else if (!strcmp(name,"uc"))       p_uc();
  else if (!strcmp(name,"index"))    p_index();
  else if (!strcmp(name,"change"))   p_change();
  else if (!strcmp(name,"split"))    p_split();
  else if (!strcmp(name,"join"))     p_join();
  //Timer e varie 
  else if (!strcmp(name,"time"))     p_time();
  else if (!strcmp(name,"date"))     p_date();
  else if (!strcmp(name,"random"))   p_random();  
  
};
