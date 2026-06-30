/////////////////////Core_di_linguaggio_amethyst/////////////////////////
#ifndef AM_REVTABLE
#define AM_REVTABLE

char *instructions[NUM_ICODES][2] = 
{ {"LDA", "q1"}, {"STA", "V1"}, {"JMP", "JA"}, {"JZ",  "zA"}, {"JN",  "nA"}, 
  {"JZN", "ZA"}, {"JP",  "pA"}, {"JES", "eA"}, {"JEQ", "QS"}, {"JLT", "TS"},
  {"JLE", "ES"}, {"NOP", ".0"}, {"CALL",">A"}, {"RET", "<0"}, {"APOP","a1"},
  {"POP", "P0"}, {"PUSH","_0"}, {"APUT","i2"}, {"SUM", "+1"}, {"SUB", "-1"},
  {"PRD", "*1"}, {"DIV", "/1"}, {"MOD", "%1"}, {"OR",  "|1"}, {"LOR", "!1"},
  {"AND", "&1"}, {"LAND","#1"}, {"NOT", "~0"}, {"NEG", "N0"}, {"PRNT","?1"},
  {"SCAT","^1"}, {"AGET","g2"}, {"DBG", "h1"}, {"JEX", "AA"}, {"INP", "I1"}, 
  {"CLOS","X0"}, {"OPN", "O1"}, {"SEN", "s0"}, {"SQR", "r0"}, {"RND", "R0"},
  {"LFT", "l2"}, {"CHR", "c0"}, {"ASC", "C0"}, {"APSH","u1"}, {"DSP", "D0"}
};

#endif
//Nota: la DBG  e' solo una istruzione di debug. Il parametro e' quello che 
//verra mostrato a video. La DBG verra' aggiunta dalla delperc in modalita'
//debug e riportera' la riga del sorgente nel compilato. 
