/*
  V 1.1.1 - 2005/04/11
    Aggiunta nuova libreria per tabella testo inserita da enrico
  V 1.1
  Seconda versione del programma di installazione. Questa versione
  lavora con il nuovo file di installazione di enrico.
*/
#define VERSION "1.1.1 2005/04/11"
#include <stdlib.h>
#include <stdio.h>
#include <io.h>

int main(int argc, char *argv[])
{
  // cerco la directory in cui sono ed estraggo il nome del drive
  char curdrive;
  char app[300];
  char windir[300];
  FILE *fp = NULL;
  
  curdrive = getcwd(NULL, 0)[0];
  // printf("\n-%c-%s\n", curdrive, getcwd(NULL, 0));

  chdir ("src");

  // printf ("%s\n", getcwd(NULL, 0));

  system("echo Amethist Installer Version "VERSION);
  system("pause");
  system("echo compilo");
  system("perl genfile ewb.p >ewb");
   
  // system("type ewpp > ewb");
  // system("type ewln >> ewb");
  // system("type ewcc >> ewb");
  system("echo compilo le librerie");
    
  chdir ("lib");
  system("perl ..\\genfile dbfiles.p > ..\\..\\dbfiles.lib");
  system("perl ..\\genfile dbmysql.p > ..\\..\\dbmysql.lib");
  system("perl ..\\genfile dbfv.p > ..\\..\\dbfv.lib");
  system("perl ..\\genfile db_dbi.p > ..\\..\\db_dbi.lib"); 
  chdir ("..");

  system("echo installo");
  
  sprintf(app, "mkdir %c:\\lib", curdrive);
  system(app);

  sprintf(app, "mkdir %c:\\lib\\ewb", curdrive);
  system(app);

  // Scrivo in un file la directory di default di windows
  sprintf(app, "echo %%windir%% > %c:\\lib\\ewb\\windir.txt", curdrive);
  system(app);

  // Ora leggo la directory di default di windows
  sprintf(app, "%c:\\lib\\ewb\\windir.txt", curdrive);

  // Ora creo un ewb.bat che punti a quello giusto
  fp = fopen(app, "r");
  if (fp != NULL)
  {
    fscanf(fp, "%s", windir);
    printf("%s", windir);
    fclose (fp);

    fp = fopen("ewb.bat", "w");
    if (fp != NULL)
    {
      fprintf(fp, "@echo off\n");
      fprintf(fp, "perl %c:\\lib\\ewb\\ewb %%1 %%2 %%3 %%4\n", curdrive);
      fclose (fp);
    }
    //copy ewb.bat %windir%\.
    sprintf(app, "copy ewb.bat %s\\ewb.bat", windir);
    system(app);

    //copy ewb.bat %windir%\amethyst.bat
    sprintf(app, "copy ewb.bat %s\\amethyst.bat", windir);
    system(app);
    
    //move ewb ..\.
    system("copy ewb ..\\ewb");

    //copy ..\ewb c:\lib\ewb\.
    sprintf(app, "copy ..\\ewb %c:\\lib\\ewb\\ewb", curdrive);
    system(app);

    //copy ..\dbfiles.lib c:\lib\ewb\.
    sprintf(app, "copy ..\\dbfiles.lib %c:\\lib\\ewb\\dbfiles.lib", curdrive);
    system(app);

    //copy ..\dbmysql.lib c:\lib\ewb\.
    sprintf(app, "copy ..\\dbmysql.lib %c:\\lib\\ewb\\dbmysql.lib", curdrive);
    system(app);

    //copy ..\db_dbi.lib c:\lib\ewb\.
    sprintf(app, "copy ..\\db_dbi.lib %c:\\lib\\ewb\\db_dbi.lib", curdrive);
    system(app);

    printf("Finito.\namethyst (ewb) installato in %c:\\lib\\ewb\n\nlibrerie in %c:\\lib\\ewb\n\n", curdrive, curdrive); 
  
    system("del ewb.bat");
    system("ewb.bat");
  }

  system("PAUSE");	
  return 0;
}
