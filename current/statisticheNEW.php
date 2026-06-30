<?PHP
/* Dati in sessione da usare sui filtri: 
   Sbrand - brand su cui si lavora, WGR o INST
   Stech  - tecnologia: "", WGR, LTE, LTEO, TIM, EOLO
   Scount - ex $mode: il calcolo richiesto: tkGuasti, all, ex, geri, ...  
   Sreseller - filtro rivenditore o  gruppi (0 tutto, 160 boc, 160000 boc_out) 
   Sanno     - annomese di inizio del calcolo nel formato YYYYmm o "" 
   Per i tickets: 
     Svis - visualizzazione mensile, giornaliera etc..
     Stipo - tipologia di ticket tutti, guasti, rallentamenti
     Sop - operatore o ''
     Stpt - tipo diagnosi: causa cliente, causa interna, tutti 
 */

//CONNECT 
$SogliaLeadsConnect=150;
$SogliaInseritiConnect=20; 

//ENERGY 
$SogliaLeadsEnergy=50; 
$SogliaInseritiEnergy=1;


//ENERGY2 
$SogliaLeadsEnergy2=200; 
$SogliaInseritiEnergy2=5;


include_once("2022/api/_contaClienti2024.php");
include_once("2022/_API_wrapper.php");
ini_set('session.gc_maxlifetime', 3*60*60);


safe_session_start(); connect();
//TOOL
/* Immagine ricerca: img/acquisti/cerca.png
 */
$cause_clienti=array(4,8,35,36,39,41,46,54,55,56,58);

//CLI section
if (strtoupper($argv[1])=="ROBOT")
{ //
  exit();
}
$PANELID=""; //Vuoto per controllo del solo accesso al GIP
check_if_logged($PANELID);


//Function section
function _head()
{ ?>
  <html lang=it>
    <head>
      <meta http-equiv="content-type" content="text/html; charset=UTF-8">
      <meta name=viewport content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0">
      <meta name="description" content="XXXYYYZZZ">
      <!-- MONTSERRAT FONT -->
      <link href="https://fonts.googleapis.com/css2?family=Montserrat:ital,wght@0,400;0,500;0,700;1,400&display=swap" rel="stylesheet">
      <!-- FAVICON -->
      <link rel="shortcut icon" href="https://ispadmin.wirelessgroup.it/pannello/favicon_gengroup.ico" type="image/x-icon">
      <!-- JS -->
      <script src="js/jquery-3.2.1.min.js"></script>
      <script src="https://code.jquery.com/ui/1.12.1/jquery-ui.js"></script>
      <script type="text/javascript" src="js/SortableTable2020.js"></script>
      <script type="text/javascript" src="js/popup2022.js"></script>
      <script src="js/chart.min.js?2"></script>
      <!-- CSS -->
      <link rel=stylesheet href="https://code.jquery.com/ui/1.12.1/themes/base/jquery-ui.css">
      <link rel="stylesheet" href="css/_popup2023.css">
      <link rel="stylesheet" href="css/statisticheNEW.css">
      <title>Statistiche intera rete</title>
    </head>
    <body>
    <header class="header">
        <div class="upper-header">
            <div class="up-header-left"><!--img src="img/logo_gengroup_white.png" alt=""--></div>
            <div class="up-header-middle"><h1>Statistiche intera rete</h1></div>
            <div class="up-header-right"><!-- <input name="cerca" value="" class="search_input"><div id="ricerca">Cerca nella pagina</div>--></div>
        </div>
        <div class="lower-header">
            <div class="low-header-left"></div>
            <div class="low-header-middle"></div>
            <div class="low-header-right"></div>
        </div>
    </header>
    <main>
      <div id=waitImage style="position: fixed;height: 40%;left: 41%;top: 50%;z-index: 10005;">
        <center><img src="img/flussoCom/loading.png?2" alt="attendi"></center>
      </div>
<script>
  function setSession(fl,val)
  { $.ajax({ url: '?ss=' + fl + '&vl=' + val, 
             success:function(result) { document.location.reload(); } 
           }); 
  };
  function openCostiLTEQ()
  { $.ajax({ url: '?costiLTEQ=1',
             success:function(result) { popupCostiLTEQ(result); }
           });
  };
  function popupCostiLTEQ(result)
  { myalertCrossBig(result);
    $('#okbutton').html('Chiudi').removeClass('procediPopup').addClass('annullaPopup');
  };
  function saveCostiLTEQ()
  { $.ajax({ type:'POST',
             url:'?saveCostiLTEQ=1',
             data:$('#formCostiLTEQ').serialize(),
             success:function(result) { popupCostiLTEQ(result); }
           });
  };
</script>
  <?
}
function tail()
{ //echo "<script>makeSortable('tabXXXZZZ');</script> ";
  echo "<script> setTimeout(\"$('#waitImage').fadeOut(1000);\",250); </script>";
  echo "</main></body></html>";
}

//AJAX section
if (isset($_GET['_xxx']))
{ //
  exit();
}

// Web section
//Headers da attivare per scaricare i dati in excel. Basta che i dati siano dentro in tabelle per ottenere un xls di base.
if (isset($_GET['ss']))
{ $ss=$_GET['ss'];
  $val=$_GET['vl']; 
  if (substr($ss,0,1)=='S')
  { $_SESSION[$ss]=$val; 
    echo "Sessione($ss) vale ora [$val]";
  };
  exit(); 
};
function canEditCostiLTEQ()
{ return in_array($_SESSION['gipUser'], Array(62,117)); }
function creaTabellaCostiLTEQ()
{ $q="create table if not exists wgr_costiLTEQ
      (annomese varchar(6) not null primary key,
       costo_iniziale_indoor decimal(10,2) null,
       costo_iniziale_outdoor decimal(10,2) null,
       ammortamento_indoor decimal(10,2) null,
       ammortamento_outdoor decimal(10,2) null,
       mesi_ammortamento int null,
       banda_indoor decimal(10,2) null,
       banda_outdoor decimal(10,2) null,
       mod_time int not null default 0,
       operatore int not null default 0)";
  mysql_query($q);
  @mysql_query("alter table wgr_costiLTEQ add mesi_ammortamento int null after ammortamento_outdoor");
}
function campiCostiLTEQ()
{ return Array
  ( "costo_iniziale_indoor"=>"Costo iniziale Qubo indoor",
    "costo_iniziale_outdoor"=>"Costo iniziale Qubo outdoor",
    "ammortamento_indoor"=>"Ammortamento Qubo indoor",
    "ammortamento_outdoor"=>"Ammortamento Qubo outdoor",
    "mesi_ammortamento"=>"Mesi di ammortamento",
    "banda_indoor"=>"Costo banda Qubo indoor",
    "banda_outdoor"=>"Costo banda Qubo outdoor"
  );
}
function mesiCostiLTEQ()
{ $res=Array();
  $ym="202501";
  $last=date("Y")."12";
  while ($ym<=$last)
  { $res[]=$ym;
    $y=intval(substr($ym,0,4)); $m=intval(substr($ym,4,2))+1;
    if ($m>12) { $m=1; $y++; };
    if ($m<10) $m="0".$m;
    $ym=$y.$m;
  };
  return $res;
}
function valoriCostiLTEQ()
{ creaTabellaCostiLTEQ();
  $campi=campiCostiLTEQ();
  $raw=Array();
  $r=perform_query("select * from wgr_costiLTEQ order by annomese");
  foreach ($r as $l) $raw[$l['annomese']]=$l;
  $eff=Array(); $last=Array("mesi_ammortamento"=>36);
  foreach (mesiCostiLTEQ() as $ym)
  { $eff[$ym]=Array();
    foreach ($campi as $campo=>$label)
    { $v=$raw[$ym][$campo];
      if ($v!=='' && $v!==null) $last[$campo]=$v;
      $eff[$ym][$campo]=$last[$campo];
    };
  };
  return Array($raw,$eff);
}
function formCostiLTEQ()
{ if (!canEditCostiLTEQ()) return "Accesso non consentito";
  $campi=campiCostiLTEQ();
  list($raw,$eff)=valoriCostiLTEQ();
  $out="<div style=text-align:left;><b>Costi Qubo</b><br><br>
        <div class=avvisoBilancio>Per il bilancio Qubo si usano importi senza IVA.</div><br><br>
        <form id=formCostiLTEQ><table id=tabCostiLTEQ>
        <tr><th>Mese</th>";
  foreach ($campi as $label) $out.="<th>$label</th>";
  $out.="</tr>";
  foreach (mesiCostiLTEQ() as $ym)
  { $out.="<tr><td>$ym</td>";
    foreach ($campi as $campo=>$label)
    { $val=$raw[$ym][$campo];
      $pl=$eff[$ym][$campo];
      $out.="<td><input type=text style=width:6em;text-align:right; name=costi[$ym][$campo] value=\"$val\" placeholder=\"$pl\"></td>";
    };
    $out.="</tr>";
  };
  $out.="</table></form>
         <br><div class=procediPopup onclick=saveCostiLTEQ();>Salva</div></div>";
  return $out;
}
function salvaCostiLTEQ()
{ if (!canEditCostiLTEQ()) return "Accesso non consentito";
  creaTabellaCostiLTEQ();
  $campi=campiCostiLTEQ();
  if (!is_array($_POST['costi'])) $_POST['costi']=Array();
  foreach ($_POST['costi'] as $ym=>$vals)
  { $ym=substr(preg_replace('/[^0-9]/','',$ym),0,6);
    if ($ym<"202501") continue;
    $sets=Array();
    $allnull=true;
    foreach ($campi as $campo=>$label)
    { $v=trim($vals[$campo]);
      if ($v==='') { $sets[]="$campo=null"; }
      else
      { $v=str_replace(",",".",$v);
        if ($campo=="mesi_ammortamento") $v=intval($v); else $v=round(floatval($v),2);
        $sets[]="$campo=$v";
        $allnull=false;
      };
    };
    if ($allnull)
    { mysql_query("delete from wgr_costiLTEQ where annomese='$ym'");
    } else
    { mysql_query("insert ignore into wgr_costiLTEQ (annomese) values ('$ym')");
      $sets[]="mod_time=".time();
      $sets[]="operatore=".intval($_SESSION['gipUser']);
      mysql_query("update wgr_costiLTEQ set ".implode(",", $sets)." where annomese='$ym'");
    };
  };
  return formCostiLTEQ();
}
function costiLTEQMese($ym)
{ list($raw,$eff)=valoriCostiLTEQ();
  $c=$eff[$ym];
  if (!is_array($c)) $c=Array();
  if (intval($c['mesi_ammortamento'])<=0) $c['mesi_ammortamento']=36;
  foreach (campiCostiLTEQ() as $campo=>$label)
  { if ($campo!="mesi_ammortamento") $c[$campo]=round(floatval($c[$campo]),2);
  };
  return $c;
}
function mesiTraLTEQ($startYm, $endYm)
{ $sy=intval(substr($startYm,0,4)); $sm=intval(substr($startYm,4,2));
  $ey=intval(substr($endYm,0,4));   $em=intval(substr($endYm,4,2));
  return (($ey-$sy)*12)+($em-$sm);
}
function riassuntoBilancioLTEQ($tot)
{ $out="<table id=detail_sum style=\"margin-bottom:1.5em;\">
        <tr><th colspan=8>Riassunto</th></tr>
        <tr>
          <th>Linee</th>
          <th>Anticipi</th>
          <th>Recuperi</th>
          <th>Insoluti</th>
          <th>Costi iniziali</th>
          <th>Ammortamento</th>
          <th>Banda</th>
          <th>Margine</th>
        </tr>
        <tr>
          <td sorttype=numeric style=text-align:right;>".$tot['clienti']."</td>
          <td sorttype=numeric style=text-align:right;>".number_format($tot['entrate'],2,",",".")."</td>
          <td sorttype=numeric style=text-align:right;>".number_format($tot['recuperi'],2,",",".")."</td>
          <td sorttype=numeric style=text-align:right;>".number_format($tot['insoluti'],2,",",".")."</td>
          <td sorttype=numeric style=text-align:right;>".number_format($tot['iniziali'],2,",",".")."</td>
          <td sorttype=numeric style=text-align:right;>".number_format($tot['ammortamento'],2,",",".")."</td>
          <td sorttype=numeric style=text-align:right;>".number_format($tot['banda'],2,",",".")."</td>
          <td sorttype=numeric style=text-align:right;>".number_format($tot['margine'],2,",",".")."</td>
        </tr>
      </table>";
  return $out;
}
function bilancioLTEQMeseInAssestamento($ym)
{ $limite=date("Ym", strtotime("-2 month"));
  return ($ym>=$limite);
}
function anticipoLTEQMese($canone, $attYm, $ym)
{ $canone=round(floatval($canone),2);
  if ($canone<=0) return 0;
  $mesi=mesiTraLTEQ($attYm, $ym);
  if ($mesi<0) return 0;
  if ($mesi==0) return round($canone*5.5,2);
  if (($mesi%2)==0) return round($canone*2,2);
  return 0;
}
function insolutoLTEQMese($idcliente, $mese, $anno)
{ $idcliente=intval($idcliente);
  $dal=strtotime($anno."-".$mese."-01 00:00:00");
  $al=strtotime("+1 month", $dal)-1;
  $q="select sum(greatest(cena_bez_dph - if(uhrazeno>0, uhrazeno/1.22, 0), 0)) as insoluto
      from billing_faktury
      where klientid=$idcliente
        and storno<1
        and zaplaceno_dat<=0
        and cena_bez_dph>0
        and dat_splatnosti between $dal and $al
        and (uhrazeno=0 or cena_s_dph>uhrazeno)";
  return round(floatval(fastquery($q)),2);
}
function canonePagatoLTEQMese($idservizio, $idcliente, $mese, $anno)
{ $idservizio=intval($idservizio);
  $idcliente=intval($idcliente);
  $dal=$anno."-".$mese."-01";
  $al=date("Y-m-t", strtotime($dal." 12:00:00"));
  $isWholesale=intval(fastquery("select count(id) from wgr_wholesales where iduser=$idcliente"));
  $pagata="bf.zaplaceno_dat>0";
  if ($isWholesale>0)
  { $pagata="1=1";
  };
  $q="select sum(bfi.cena_bez_dph /
          greatest(1, timestampdiff(month, bfi.invoiced_from, date_add(bfi.invoiced_to, interval 1 day)))) as canone
      from billing_faktury_item bfi
      join billing_faktury bf on bf.id=bfi.faktid
      where bfi.servicetype=1
        and bfi.serviceid=$idservizio
        and bfi.invoiced_from<='$al'
        and bfi.invoiced_to>='$dal'
        and bf.storno<1
        and $pagata";
  return round(floatval(fastquery($q)),2);
}
function clienteLTEQBandaAttiva($idcliente, $monthEnd)
{ $idcliente=intval($idcliente);
  $dis=intval(fastquery("select min(if(official_time>0, official_time, if(close_time>0, close_time, if(block_time>0, block_time, ins_time))))
                         from wgr_disdette where idcliente=$idcliente"));
  if ($dis>0 && $dis<=$monthEnd) return false;
  $geri=intval(fastquery("select ins_time from wgr_geri where id=$idcliente and ins_time>0 order by ins_time limit 1"));
  if ($geri>0 && $geri<=$monthEnd) return false;
  return true;
}
function bilancioLTEQMese($mese, $anno)
{ $ym=$anno.$mese;
  $monthStart=strtotime($anno."-".$mese."-01 00:00:00");
  $monthEnd=strtotime("+1 month", $monthStart)-1;
  $costi=costiLTEQMese($ym);
  $q="select si.id as idservizio, si.klientid, si.cena, si.vernostni_sleva, si.pripojen_od, si.disabled, si.disabled_from,
             cu.username, cu.firma_fakt, cu.active_time,
             ct.codicepannello, ct.popis
      from sl_internet si
      join cable_tarif ct on ct.id=si.tarif
      join cable_users cu on cu.id=si.klientid
      where ct.codicepannello in ('LTEQ-F','LTEQ-A','LTEQ-FO','LTEQ-AO')
        and si.pripojen_od<=$monthEnd
        and (si.visible>0 or si.disabled_from>=$monthStart)
        and (si.disabled<1 or si.disabled_from=0 or si.disabled_from>=$monthStart)
      order by ct.codicepannello, cu.username, si.id";
  $r=perform_query($q);
  $tot=Array("entrate"=>0, "recuperi"=>0, "insoluti"=>0, "iniziali"=>0, "ammortamento"=>0, "banda"=>0, "margine"=>0, "clienti"=>0);
  $righe="";
  $n=1;
  $insolutiClienti=Array();
  foreach ($r as $l)
  { $att=intval($l['active_time']);
    if ($att<=0) $att=intval($l['pripojen_od']);
    if ($att<=0 || $att>$monthEnd) continue;
    $attYm=date("Ym", $att);
    $mesi=mesiTraLTEQ($attYm, $ym);
    $outdoor=in_array($l['codicepannello'], Array("LTEQ-FO","LTEQ-AO"));
    $canoneMensile=round(floatval($l['cena'])-floatval($l['vernostni_sleva']),2);
    if ($canoneMensile<0) $canoneMensile=0;
    $canone=anticipoLTEQMese($canoneMensile, $attYm, $ym);
    $insoluto=0;
    if (!isset($insolutiClienti[$l['klientid']]))
    { $insolutiClienti[$l['klientid']]=insolutoLTEQMese($l['klientid'], $mese, $anno);
    };
    $insoluto=$insolutiClienti[$l['klientid']];
    $insolutiClienti[$l['klientid']]=0;
    $iniziale=0;
    if ($attYm==$ym)
    { $iniziale=$outdoor ? $costi['costo_iniziale_outdoor'] : $costi['costo_iniziale_indoor'];
    };
    $amm=0;
    if ($mesi<intval($costi['mesi_ammortamento']))
    { $amm=$outdoor ? $costi['ammortamento_outdoor'] : $costi['ammortamento_indoor'];
    };
    $banda=0;
    if (clienteLTEQBandaAttiva($l['klientid'], $monthEnd))
    { $banda=$outdoor ? $costi['banda_outdoor'] : $costi['banda_indoor'];
    };
    $margine=round($canone-$insoluto-$iniziale-$amm-$banda,2);
    $nome=$l['username'];
    if ($nome=="") $nome=$l['firma_fakt'];
    $tot['entrate']+=$canone;
    $tot['iniziali']+=$iniziale;
    $tot['insoluti']+=$insoluto;
    $tot['ammortamento']+=$amm;
    $tot['banda']+=$banda;
    $tot['margine']+=$margine;
    $tot['clienti']++;
    $righe.="<tr>
      <td sorttype=numeric>$n</td>
      <td>".$nome." (".$l['klientid'].")</td>
      <td>".$l['popis']."</td>
      <td sorttype=numeric style=text-align:right;>".number_format($canone,2,",",".")."</td>
      <td sorttype=numeric style=text-align:right;>0,00</td>
      <td sorttype=numeric style=text-align:right;>".number_format($insoluto,2,",",".")."</td>
      <td sorttype=numeric style=text-align:right;>".number_format($iniziale,2,",",".")."</td>
      <td sorttype=numeric style=text-align:right;>".number_format($amm,2,",",".")."</td>
      <td sorttype=numeric style=text-align:right;>".number_format($banda,2,",",".")."</td>
      <td sorttype=numeric style=text-align:right;>".number_format($margine,2,",",".")."</td>
      <td sorttype=numeric>".date("m/Y", $att)."</td>
    </tr>";
    $n++;
  };
  $q="select gr.idcliente, gr.data, gr.cifra, cu.username, cu.firma_fakt,
             (select ct.popis from sl_internet si join cable_tarif ct on ct.id=si.tarif
              where si.klientid=gr.idcliente and ct.codicepannello in ('LTEQ-F','LTEQ-A','LTEQ-FO','LTEQ-AO')
              order by si.pripojen_od desc limit 1) as linea
      from wgr_geriRecuperi gr
      join cable_users cu on cu.id=gr.idcliente
      where gr.data like '__-$mese-$anno'
        and gr.idcliente in
        (select distinct si.klientid from sl_internet si join cable_tarif ct on ct.id=si.tarif
         where ct.codicepannello in ('LTEQ-F','LTEQ-A','LTEQ-FO','LTEQ-AO'))
      order by gr.data, gr.idcliente";
  $r=perform_query($q);
  foreach ($r as $l)
  { $rec=round(floatval($l['cifra']),2);
    if ($rec==0) continue;
    $nome=$l['username'];
    if ($nome=="") $nome=$l['firma_fakt'];
    $tot['recuperi']+=$rec;
    $tot['margine']+=$rec;
    $righe.="<tr>
      <td sorttype=numeric>$n</td>
      <td>".$nome." (".$l['idcliente'].")</td>
      <td>".$l['linea']." - Recupero crediti ".$l['data']."</td>
      <td sorttype=numeric style=text-align:right;>0,00</td>
      <td sorttype=numeric style=text-align:right;>".number_format($rec,2,",",".")."</td>
      <td sorttype=numeric style=text-align:right;>0,00</td>
      <td sorttype=numeric style=text-align:right;>0,00</td>
      <td sorttype=numeric style=text-align:right;>0,00</td>
      <td sorttype=numeric style=text-align:right;>0,00</td>
      <td sorttype=numeric style=text-align:right;>".number_format($rec,2,",",".")."</td>
      <td sorttype=numeric>-</td>
    </tr>";
    $n++;
  };
  $det="<div style=text-align:left;><b>Bilancio Qubo $mese/$anno</b><br><br>";
  $det.="<div class=avvisoBilancio>Entrate: anticipo bancario Qubo indoor, 5,5 mensilita' al mese di attivazione e poi 2 mensilita' ogni due mesi.</div>";
  $det.="<div class=avvisoBilancio>Dal mese si sottraggono le fatture con scadenza nel mese che risultano ancora impagate ad oggi.</div><br><br>";
  if (bilancioLTEQMeseInAssestamento($ym))
  { $det.="<div class=avvisoBilancio>Attenzione: parte delle fatture di questo mese potrebbero non essere ancora pagate e quindi il bilancio cambiare nel tempo.</div><br><br>";
  };
  $det.=riassuntoBilancioLTEQ($tot);
  $det.="<table id=detail>
        <tr><th>#</th><th>Cliente</th><th>Linea</th><th>Anticipo</th><th>Recuperi</th><th>Insoluti</th><th>Costo iniziale</th><th>Ammortamento</th><th>Banda</th><th>Margine</th><th>Mese att.</th></tr>";
  $det.=$righe;
  $det.="<tr><th colspan=3>Totale</th>
    <th style=text-align:right;>".number_format($tot['entrate'],2,",",".")."</th>
    <th style=text-align:right;>".number_format($tot['recuperi'],2,",",".")."</th>
    <th style=text-align:right;>".number_format($tot['insoluti'],2,",",".")."</th>
    <th style=text-align:right;>".number_format($tot['iniziali'],2,",",".")."</th>
    <th style=text-align:right;>".number_format($tot['ammortamento'],2,",",".")."</th>
    <th style=text-align:right;>".number_format($tot['banda'],2,",",".")."</th>
    <th style=text-align:right;>".number_format($tot['margine'],2,",",".")."</th>
    <th>".$tot['clienti']." linee</th></tr>";
  $det.="</table></div>";
  return Array($tot,$det);
}
if (isset($_GET['costiLTEQ']))
{ echo formCostiLTEQ();
  exit();
};
if (isset($_GET['saveCostiLTEQ']))
{ echo salvaCostiLTEQ();
  exit();
};
if (isset($_GET['excel']))
{ header("Content-type: application/vnd.ms-excel");
  header("Content-Disposition: attachment; filename=xxxYYY.xls");
}


//--------------------------------------------------------------------------//
//---------------------------Codice-dei-tickets-----------------------------//
//--------------------------------------------------------------------------//

function guastiSezDx()
{ $USEDBRAND=$_SESSION['Sbrand']; if ($USEDBRAND=='') $USEDBRAND='WGR';
  $USEDTECH=$_SESSION['Stech']; if ($USEDTECH=='') $USEDBRAND='WGR';
  $mode="tkGuasti"; 
  $vis=$_SESSION['Svis'];
  $oper=$_SESSION['Sop'];
  $tipo=$_SESSION['Stipo']; 
  $tpt=$_SESSION['Stpt']; 
  $_mkt='cli';
  $exp=0;
  $vel=""; 
  $datiout='';
  if ($USEDBRAND=='WGR')
  { $out=_workOnTickets($USEDBRAND, $USEDTECH, $mode, $vis, $_oper, $tipo, $tpt, $_mkt, $exp, $vel);
    if ( preg_match('/^WGR$|^LTEx$|^FTTx$|^$/', $USEDTECH) && $mode=='tkGuasti' && $vis=='')
    { $datiout=tabDaTorta($out['tab_gestiti_tip_2'], 'Tickets di '.labelMesi(date("m"), date("Y"), 2));
      $datiout.=tabDaTorta($out['tab_gestiti_tip_1'], 'Tickets di '.labelMesi(date("m"), date("Y"), 1),'');
      $datiout.=tabDaTorta($out['tab_gestiti_tip_0'], 'Tickets di '.labelMesi(date("m"), date("Y"), 0),$out['tab_gestiti_tip_1']);

      //Faccio la classifica dei tipi piu usati.
     $tot=Array();
     $nttickets=0;  
     foreach ($out['tab_gestiti_tip_2'] as $causa=>$d)
     { $tot[$causa]=$tot[$causa] + $d[0];
       $nttickets=$nttickets+$d[0];
     };
     foreach ($out['tab_gestiti_tip_1'] as $causa=>$d)
     { $tot[$causa]=$tot[$causa] + $d[0]; 
       $nttickets=$nttickets+$d[0];
     };
     foreach ($out['tab_gestiti_tip_0'] as $causa=>$d)
     { $tot[$causa]=$tot[$causa] + $d[0]; 
       $nttickets=$nttickets+$d[0];
     };
     $m1=""; $m2=""; $m3=""; 
     $qm1=0; $qm2=0; $qm3=0; 
     foreach ($tot as $a=>$b) 
     { if (($b>$qm1) && ($a!=$m1)) 
       { $m3=$m2; $m2=$m1; 
         $qm3=$qm2; $qm2=$qm1; 
         $m1=$a; $qm1=$b; 
       } else if (($b>$qm2) && ($a!=$m2)) 
       { $m3=$m2;  
         $qm3=$qm2;  
         $m2=$a; $qm2=$b; 
       } else if (($b>$qm3) && ($a!=$m3))
       { $m3=$a; $qm3=$b; 
       }; 
     };

     echo "<div class=statointremesi>"; 
     echo "Prima causa: <b>$m1</b>, <b>$qm1</b> problemi in 3 mesi, <b>".round(($qm1*100/$nttickets),1)."%</b> del totale<br>\n";   
     echo "Seconda causa: <b>$m2</b>, <b>$qm2</b> problemi in 3 mesi, <b>".round(($qm2*100/$nttickets),1)."%</b> del totale<br>\n";   
     echo "Terza causa: <b>$m3</b>, <b>$qm3</b> problemi in 3 mesi, <b>".round(($qm3*100/$nttickets),1)."%</b> del totale<br>\n";   
     echo "</div>"; 

    }
  }
  return $datiout;
}
//Views generiche e per il conteggio dei clienti
function labelMesi($mese, $anno, $type)
{ $mesi=Array("","Gennaio","Febbraio","Marzo","Aprile","Maggio","Giugno","Luglio","Agosto","Settembre","Ottobre","Novembre","Dicembre");
  if ($type==0)
  { $nome=$mesi[intval($mese)];
    return "$nome $anno";
  }
  if ($type==1)
  { $date = new DateTime($anno.'-'.$mese.'-01 00:00:00');
    $date->modify('-1 month');
    $nome=$mesi[intval($date->format('m'))];
    $year=$date->format('Y');
    return "$nome $year";
  }
  if ($type==2)
  { $date = new DateTime($anno.'-'.$mese.'-01 00:00:00');
    $date->modify('-2 month');
    $nome=$mesi[intval($date->format('m'))];
    $year=$date->format('Y');
    return "$nome $year";
  }
}
function valutazioneGuasti($USEDBRAND, $USEDTECH,$mode, $_oper, $ins, $gst, $by_operator, $exp, $vis )
{ if (($mode =='tkGuasti') && ($USEDBRAND=='WGR') && ( ($USEDTECH=='') || ($USEDTECH=='WGR')) && ($_oper==0)  && ($exp==0) && $vis=='') //Solo Guasti 'veri' e per WGR adesso
  { global $MAXGUASTI_WGR, $cause_clienti;
    if (($USEDTECH=='') || ($USEDTECH=='WGR') ) { $max_guasti=$MAXGUASTI_WGR;}
    $n_array=(count($gst)-1);
    //print_r($n_array);
    //exit();
    $sotto="<table class=premi><tr>";
    //print_r($by_operator[0]);
    //print_r($ins);
    //exit();
    for ($i=0; $i<$n_array; $i++)
    { if ($ins[$i]<$max_guasti)
      { $sotto.="<td>";
        foreach ($by_operator[$i] as $oper)  { $sotto.=$oper['nome'].": ".$oper['ngst']." ".divChart($oper['ngst'],$gst[$i])."% <br/>"; }
        $sotto.="</td>";
      }   else
      { $sotto.="<td class=negativo>";
        foreach ($by_operator[$i] as $oper)  { $sotto.=$oper['nome'].": ".$oper['ngst']." ".divChart($oper['ngst'],$gst[$i])."% <br/>"; }
        $sotto.="</td>";
      }
    };

    if ($ins[$n_array]<$max_guasti)
    { $sotto.="<td>Parziale: $ins[$n_array]<br/>";
      foreach ($by_operator[$n_array] as $oper)  { $sotto.=$oper['nome'].": ".$oper['ngst']." ".divChart($oper['ngst'],$gst[$i])."% <br/>"; }
      $sotto.="</td>";
    } else
    { $sotto.="<td class=negativo>Parziale: $ins[$n_array]<br/>";
      foreach ($by_operator[$n_array] as $oper)  {$sotto.=$oper['nome'].": ".$oper['ngst']." ".divChart($oper['ngst'],$gst[$i])."% <br/>"; }
      $sotto.="</td>";
    }
    $sotto.="</tr></table>";
    echo $sotto;
    //Aggiungiamo una seconda sezione con il numero di ticket scaduti nel mese.
    $sotto="<table class=premi><tr>";
    $y=date("y")-1;
    $m=date("m");  
    $m++; if ($m>12) { $y++; $m=$m-12; };
    if (strlen($m)==1) $m='0'.$m; 
    global $cause_clienti;
    for ($i=0; $i<12; $i++)
    { //Totale tickets mese esclusi causa cliente 
      $sotto.="<td>";
      $tottk=fastquery("select count(id) from wgr_tickets where 
        diagnosi not in (".implode(",",$cause_clienti).") and id like '".$y.$m."%'  and title not like '%non funzionante%'"); 
      $totguasti=fastquery("select count(id) from wgr_tickets where 
        diagnosi not in (".implode(",",$cause_clienti).") and id like '".$y.$m."%' and title like '%non funzionante%'"); 
      $tottkd=fastquery("select count(id) from wgr_tickets where 
        diagnosi not in (".implode(",",$cause_clienti).") and id like '".$y.$m."%' and scadutoil>0  and title not like '%non funzionante%'"); 
      $totguastid=fastquery("select count(id) from wgr_tickets where 
        diagnosi not in (".implode(",",$cause_clienti).") and id like '".$y.$m."%' and title like '%non funzionante%'
        and scadutoil>0"); 
      if ($totguasti>0 && $tottk>0) 
      { $sotto.="<div style=font-size:180%;display:inline-block;>".intval(($totguastid*100)/$totguasti)."%</div> guasti scaduti<br>".
                "<div style=font-size:180%;display:inline-block;>".intval(($tottkd*100)/$tottk)."%</div> altro scaduti<br>";    
      };  
      $sotto."</td>";
      $m++; 
      if ($m>12) { $m=$m-12; $y++; };
      if (strlen($m)==1) $m='0'.$m; 
    };
    $sotto.="</tr></table>";
    echo $sotto; 
  }
}
function _workOnTickets($USEDBRAND, $USEDTECH, $mode, $vis, $_oper, $tipo, $tpt, $_mkt, $exp, $vel)
{ $by_operator=array();
  $ins = array();
  $gst = array();
  $x = array();
  $dati=array();
  $tab_gestiti_tip_0=array();
  $tab_gestiti_tip_1=array();
  $tab_gestiti_tip_2=array();

  //Estrazione dati
  if ($vis == "")
  { for ($i = 11; $i >= 0; $i--) //Mensile su 12 mesi
    { $startdate= date("Y-m").'-01'; $starepoch=strtotime($startdate);
      $dt = date("Y-m", strtotime("-$i month", $starepoch ));
      $dn = explode("-", $dt);
      $dn = $dn[1] . "-" . $dn[0];
      $x[] = $dn;
      $q="select id, idclient, idtype, title, diagnosi, open_by, close_by, close_time AS time_close,
                COALESCE((SELECT line from wgr_contracts WHERE numero=idclient),  
                         'WGR') AS line
              from wgr_tickets 
              where (idtype=1 OR idtype=15) and from_unixtime(close_time,'%Y-%m')='$dt'
              UNION
              select id, idclient, idtype, title, diagnosi, open_by, close_by, gest_time AS time_close,
                COALESCE((SELECT line from wgr_contracts WHERE numero=idclient),  
                         'WGR') AS line
              from wgr_tickets 
              where (idtype=1 OR idtype=15) and close_time=0 and from_unixtime(gest_time,'%Y-%m')='$dt'";
      $gestiti=perform_query($q);
      $q="select id, idclient, idtype, title, diagnosi, open_by, close_by, close_time AS time_close, gest_time,
                COALESCE((SELECT line from wgr_contracts WHERE numero=idclient),  
                         'WGR') AS line
              from wgr_tickets 
              where (idtype=1 OR idtype=15) and from_unixtime(ins_time,'%Y-%m')='$dt'";
      $inseriti=perform_query($q);
      /*if($i==0){ $tab_gestiti_tip_0=filtraEdividiGuastiTipi($gestiti,$USEDBRAND, $USEDTECH,1);}
      if($i==1){ $tab_gestiti_tip_1=filtraEdividiGuastiTipi($gestiti,$USEDBRAND, $USEDTECH,1);}
      if($i==2){ $tab_gestiti_tip_2=filtraEdividiGuastiTipi($gestiti,$USEDBRAND, $USEDTECH,1);}*/
      if($i==0){ $tab_gestiti_tip_0=filtraEdividiGuastiTipi($inseriti,$USEDBRAND, $USEDTECH,1);}
      if($i==1){ $tab_gestiti_tip_1=filtraEdividiGuastiTipi($inseriti,$USEDBRAND, $USEDTECH,1);}
      if($i==2){ $tab_gestiti_tip_2=filtraEdividiGuastiTipi($inseriti,$USEDBRAND, $USEDTECH,1);}
      if ($exp == 0)
      { //$dati=filtraEContaGuasti($gestiti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,1);
        $dati=filtraEContaGuasti($inseriti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,1);
        $gst[]=$dati['conta'];
        $by_operator[]=$dati['op'];
      }
      if ($exp == 1) { $dati[]=filtraEdividiGuastiTipi($gestiti,$USEDBRAND, $USEDTECH);}
      if ($exp == 0)
      { $ins[]=filtraEContaGuasti($inseriti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,0);
        if ($USEDTECH=='')
        { $ins_WGR[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'WGR',$tipo,$tpt,$_oper,0);
          $ins_LTEx[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'LTEx',$tipo,$tpt,$_oper,0);
          $ins_FTTx[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'FTTx',$tipo,$tpt,$_oper,0);
        }
      }
   };
  } else if ($vis == 'g') //giornaliera su ultimi 31 giorni
  { for ($i = 30; $i >= 0; $i--)
    { $dt = date("Y-m-d", strtotime("-$i day", time()));
      $dn = explode("-", $dt);
      $dn = $dn[2] . "-" . $dn[1] . "-" . $dn[0];
      $x[] = $dn;
      $q="select id, idclient, idtype, title, diagnosi, open_by, close_by, close_time AS time_close,
                COALESCE((SELECT line from wgr_contracts WHERE numero=idclient),  
                         'WGR') AS line
              from wgr_tickets 
              where (idtype=1 OR idtype=15) and from_unixtime(close_time,'%Y-%m-%d')='$dt'
              UNION
              select id, idclient, idtype, title, diagnosi, open_by, close_by, gest_time AS time_close,
                 COALESCE((SELECT line from wgr_contracts WHERE numero=idclient),  
                         'WGR') AS line
              from wgr_tickets 
              where (idtype=1 OR idtype=15) and close_time=0 and from_unixtime(gest_time,'%Y-%m-%d')='$dt'";
      $gestiti=perform_query($q);
      $q="select id, idclient, idtype, title, diagnosi, open_by, close_by, close_time AS time_close,
                COALESCE((SELECT line from wgr_contracts WHERE numero=idclient),  
                         'WGR') AS line
              from wgr_tickets 
              where (idtype=1 OR idtype=15) and from_unixtime(ins_time,'%Y-%m-%d')='$dt'";
      $inseriti=perform_query($q);
      if ($exp == 0)
      { //$dati=filtraEContaGuasti($gestiti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,1);
        $dati=filtraEContaGuasti($inseriti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,1);
        $gst[]=$dati['conta'];
        $by_operator[]=$dati['op'];
      }
      if ($exp == 1) { $dati[]=filtraEdividiGuastiTipi($gestiti,$USEDBRAND, $USEDTECH);}
      if ($exp == 0)
      { $ins[]=filtraEContaGuasti($inseriti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,0);
        if ($USEDTECH=='')
        { $ins_WGR[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'WGR',$tipo,$tpt,$_oper,0);
          $ins_LTEx[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'LTEx',$tipo,$tpt,$_oper,0);
          $ins_FTTx[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'FTTx',$tipo,$tpt,$_oper,0);
        }
      }
    };
  } else if ($vis == 's') //Settimanale un po piu complessa, ultime 10 settimane
  { //Trovo questo lunedi e tolgo N settimane
    $tm = time();
    while (date("w", $tm) != 1) { $tm = $tm - 24 * 3600; }
    for ($i = 9; $i >= 0; $i--)
    { $dt = date("Y-m-d", strtotime("-$i week", $tm));
      $dtb = date("Y-m-d", strtotime("-$i week", $tm + 24 * 3600 * 6));
      $dn = explode("-", $dt);
      $dn = $dn[2] . "-" . $dn[1] . "-" . $dn[0];
      $x[] = $dn;
      $q="select id, idclient, idtype, title, diagnosi, open_by, close_by, close_time AS time_close,
               COALESCE((SELECT line from wgr_contracts WHERE numero=idclient),  
                         'WGR') AS line
            from wgr_tickets 
            where (idtype=1 OR idtype=15) and from_unixtime(close_time,'%Y-%m-%d')>='$dt' and from_unixtime(close_time,'%Y-%m-%d')<='$dtb' 
            UNION
            select id, idclient, idtype, title, diagnosi, open_by, close_by, gest_time AS time_close,
               COALESCE((SELECT line from wgr_contracts WHERE numero=idclient),  
                         'WGR') AS line
            from wgr_tickets 
            where (idtype=1 OR idtype=15) and close_time=0 and  from_unixtime(gest_time,'%Y-%m-%d')>='$dt' and from_unixtime(gest_time,'%Y-%m-%d')<='$dtb'";
      $gestiti=perform_query($q);
      if ($exp == 1) { $dati[]=filtraEdividiGuastiTipi($gestiti,$USEDBRAND, $USEDTECH);}
      $q="select id, idclient, idtype, title, diagnosi, open_by, close_by, close_time AS time_close,
               COALESCE((SELECT line from wgr_contracts WHERE numero=idclient), 
                         'WGR') AS line
            from wgr_tickets 
            where (idtype=1 OR idtype=15) and from_unixtime(ins_time,'%Y-%m-%d')>='$dt' and from_unixtime(ins_time,'%Y-%m-%d')<='$dtb'";
      $inseriti=perform_query($q);
      if ($exp == 0)
      { //$dati=filtraEContaGuasti($gestiti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,1);
        $dati=filtraEContaGuasti($inseriti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,1);
        $gst[]=$dati['conta'];
        $by_operator[]=$dati['op'];
      }
      if ($exp == 0)
      { $ins[]=filtraEContaGuasti($inseriti,$USEDBRAND, $USEDTECH,$tipo,$tpt,$_oper,0);
        if ($USEDTECH=='')
        { $ins_WGR[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'WGR',$tipo,$tpt,$_oper,0);
          $ins_LTEx[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'LTEx',$tipo,$tpt,$_oper,0);
          $ins_FTTx[]=filtraEContaGuasti($inseriti,$USEDBRAND, 'FTTx',$tipo,$tpt,$_oper,0);
        }
      }
    };
  };


  $all_y = array();
  if (($exp==0) && ($_oper==0) & ($USEDTECH==''))
  { //$all_y=array(array('Tutti i ticket inseriti', $ins),array('Ticket WGR', $ins_WGR),array('Ticket Qubo', $ins_LTEx),array('Ticket FTTx', $ins_FTTx));
    $all_y=array(array('Tutti i ticket inseriti',$ins),array('Ticket WGR', $ins_WGR),array('Ticket Qubo', $ins_LTEx),array('Ticket FTTx', $ins_FTTx));
    if ((($tpt==-2) || ($tpt==0)) && ($vis == "")) { $all_y[]=array('Limite tickets',array(150,150,150,150,150,150,150,150,150,150,150,150)); }
  }
  if (($exp==0) && ($_oper==0) & ($USEDTECH!=''))
  { $all_y=array( array('Ticket inseriti', $ins ) );}

  if (($exp==0) && ($_oper>0)) { $all_y=array( array('ticket chiusi o gestiti', $gst ) ); }
  if ($exp==1) { $all_y=riconfiguraDiagnosi($dati);}


  plotChartArea();
  valutazioneGuasti($USEDBRAND, $USEDTECH,$mode, $_oper, $ins, $gst, $by_operator, $exp, $vis);
  legendaTickets($USEDBRAND, $USEDTECH, $mode, $_oper, $exp, $vis);
  mostraGrafico($x, $all_y);

  $totale=array('tab_gestiti_tip_0'=>$tab_gestiti_tip_0, 'tab_gestiti_tip_1'=> $tab_gestiti_tip_1, 'tab_gestiti_tip_2'=>$tab_gestiti_tip_2);
  return $totale;
}
function tabDaTorta($res, $title, $confronto='')
{ $out="<div class=tortadisplay>
        <div class=titolotorta>$title</div>
         <table class=torta>
          <tr>
            <th>Diagnosi</th>
            <th>Numero</th>
            <th>Percentuale</th>
          </tr>";
  if($confronto=='')
  { foreach ($res as $x => $a)
    { $out.= "<tr>
             <td>$x</td>
             <td style=text-align:right;>" . $a[0] . "</td>
             <td style=text-align:right;>" . $a[1] . "%</td>
             </tr>";
    };
  }
  if($confronto!='')
  { foreach ($res as $x => $a)
    { $chk=$confronto[$x];
      if ($a[0]==0) {$style="style=text-align:right;";}
      elseif ($chk[1]>$a[1]) {$style="style=text-align:right;"; }
      else { $style="style=text-align:right;color:red"; }
      $out.= "<tr>
             <td>$x</td>
             <td $style>" . $a[0] . "</td>
             <td $style>" . $a[1] . "%</td>
             </tr>";
    }
  }
  $out.="</table></div>";
  return $out;
}



//--------------------------------------------------------------------------//
//---------------------------Codice-dei-grafici-----------------------------//
//-------------------Cerco-di-uniformare-e-rendere-comodo-------------------//
//--------------------------------------------------------------------------//

$actual_id_grafico=''; 
function preparaGrafico($title, $data, $dal, $weekly=false)
{ //$title -> il titolo
  //Dati: per ciascuna riga ha una label e i valori.
  //      può esserci un dettaglio che riguarda l'intero grafico.
  // $data = Array(Label -> Array(value), { "detail"=>Array(detail) } );   
//echo "preparaGrafico($title, $data, $dal, $weekly);<br>"; 
  global $actual_id_grafico; 
  $tempi=Array(); 
  $dl=explode("-", $dal); 
  $dal=strtotime($dal." 12:00:00");
  for ($i=0; $i<12; $i++)
  {  
    if ($weekly) { $tempi[]=$dl[2]."/".$dl[1]; $dal=strtotime("+1 week", $dal);
                 } else { $tempi[]=$dl[1]."/".$dl[0]; $dal=strtotime("+1 month", $dal); };
    $dl=explode("-", date("Y-m-d", $dal)); 
  };

  if (intval($actual_id_grafico)==0) $actual_id_grafico=0; 
  $we="(mensile)"; if ($weekly) $we="(settimanale)"; 
  echo "
    <div class=canvas-title><b>$title</b> $we</div>
    <div id=canvas-holder$actual_id_grafico style=\"width: calc(100vw - 20em);\">
      <div class=chartjs-size-monitor>
        <div class=chartjs-size-monitor-expand>
          <div></div>
        </div>
        <div class=chartjs-size-monitor-shrink>
          <div></div>
        </div>
      </div>
      <canvas id=chart-area$actual_id_grafico
         style=\"display: inline-block; height: 371px; width: 100%; background-color:white;\" 
         class=chartjs-render-monitor></canvas>
    </div>";
  //Sezione di dettaglio, se presente sul grafico 
  if (is_array($data['detail'])) 
  { $detailName="Dettaglio"; if (isset($data['detailName'])) $detailName=$data['detailName']; 
    echo "<table style=width:100%;><tr>"; 
    for ($i=0; $i<12; $i++)
    { $det=$data['detail'];
      $dt=$det[$i]; 
      $dt=implode('`', explode("'", $dt)); 
      $dt=implode('`', explode('"', $dt)); 
      $dt=implode(' ', explode("\n", $dt)); 
      echo "<td class=btn_dettaglio onclick=\"myalertCrossBig('$dt');makeSortable('detail');\">$detailName</td>"; 
    };
    echo "</tr></table>"; 
  };
  if (is_array($data['detail_ko']))
  { echo "<table style=width:100%;><tr>"; 
    for ($i=0; $i<12; $i++)
    { $det=$data['detail_ko'];
      $dt=$det[$i]; 
      $dt=implode('`', explode("'", $dt)); 
      $dt=implode('`', explode('"', $dt)); 
      $dt=implode(' ', explode("\n", $dt)); 
      echo "<td class=btn_dettaglio onclick=\"myalertCrossBig('$dt');makeSortable('detail_ko');\">%Ko</td>"; 
    };
    echo "</tr></table>"; 
  };
  if (is_array($data['detail_int']))
  { echo "<table style=width:100%;><tr>"; 
    for ($i=0; $i<12; $i++)
    { $det=$data['detail_int'];
      $dt=$det[$i]; 
      $dt=implode('`', explode("'", $dt)); 
      $dt=implode('`', explode('"', $dt)); 
      $dt=implode(' ', explode("\n", $dt)); 
      echo "<td class=btn_dettaglio onclick=\"myalertCrossBig('$dt');makeSortable('detail_int');\">%Int</td>"; 
    };
    echo "</tr></table>"; 
  };
  //Un po di colori pronti, al momento per otto righe, dovrebbero bastare. 
  $dscolors=Array(Array("rgba(255,0,0,0)",     "rgba(255,0,0,1)"), 
                  Array("rgba(220,220,0,0)",   "rgba(220,220,0,1)"),
                  Array("rgba(0,0,220,0)",     "rgba(0,0,220,1)"), 
                  Array("rgba(0,220,0,0)",     "rgba(0,220,0,1)"),
                  Array("rgba(100,100,200,0)", "rgba(100,100,200,1)"), 
                  Array("rgba(200,0,100,0)", "rgba(200,0,100,1)"),
                  Array("rgba(120,120,120,0)", "rgba(120,120,120,1)"), 
                  Array("rgba(100,200,100,0)", "rgba(100,200,100,1)")
                 ); 
  $labels=Array(); 
  $datasets=Array(); 
  $cnt=0; 
  foreach ($data as $a=>$b) 
  { if ($a!="detail" && $a!="detailName" && $a!="detail_ko"  && $a!="detail_int") 
    { $col=$dscolors[$cnt%8]; 
      $datasets[]="
          { backgroundColor: \"".$col[0]."\", borderColor:\"".$col[1]."\",label:\"$a\", linetension:0,
            data: [".implode(",", $b)."]  
          }"; 
    };
    $cnt++; 
  }; 
  echo "
  <script>     
    var config$actual_id_grafico=
    { type: 'line',
      data: 
      { labels: [\"".implode("\",\"", $tempi)."\"],
        datasets: 
        [ 
          ".implode(",\n", $datasets)."
        ] 
      }, 
      options: { steppedLine: true }
    };
  </script>\n";
  $actual_id_grafico++;                  
};
function avviaGrafici()
{ global $actual_id_grafico; 
  echo "<script>
  window.onload = function() 
  { $('.ascomparsa').fadeOut(1000); 
  \n"; 
  for ($i=0; $i<$actual_id_grafico; $i++)
  { echo "
    var ctx = document.getElementById('chart-area$i').getContext('2d');
    window.myPie = new Chart(ctx, config$i);"; 
  };
  echo "
  }
  function showGraphicDetail(graph,index,ds)
  { //Mostro sempre il dettaglio delle voci che hanno portato al grafico.
    $.ajax
    ({ url: '?graphicdetail=' + graph + '&index=' + index, 
       success: function(result) { myalert(result);  makeSortable('detail'); }
    });
  };
</script><br><br><br><br>\n"; 
};
//Questo per i tickets.
function plotChartArea()
{ echo '
  <div id="canvas-holder" style="width:96%;">
    <div class="chartjs-size-monitor">
      <div class="chartjs-size-monitor-expand">
        <div class=""></div>
      </div>
      <div class="chartjs-size-monitor-shrink">
        <div class=""></div>
      </div>
    </div>
    <canvas id="chart-area" style="display: inline-block; height: 371px; width: 100%; background-color:white;" class="chartjs-render-monitor" ></canvas>
  </div>';
}
function legendaTickets($USEDBRAND, $USEDTECH,$mode, $_oper, $exp, $vis)
{ if ($USEDBRAND=='WGR')
  { if (($mode =='tkGuasti') && ($USEDTECH== '') && $_oper==0 && $exp==0  && $vis=='')
    { echo "<div class=legendaSoglia>% ticket chiusi o gestiti per op (nel mese), la soglia &egrave; di 150 tickets/mese aperti esclusi guasti a   carico del cliente</div><br/>"; 
      echo "<div class=legendaSoglia><a href='DashboardGuasti.php?gmap' target='_blank'>Mappa dei Tickets</a></div><br/>";  
    }
  }
}
function mostraGrafico($x, $y)
{ $colors_bk=Array("rgba(0,0,0,0)","rgba(240,94,35,0)","rgba(50,255,50,0)", "rgba(180,67,29,0)", "rgba(255,0,0,0)", "rgba(148,0,211,0)", "rgba(119,136,153,0)", "rgba(127,255,212,0)", "rgba(107,142,35,0)", "rgba(255,215,0,0)" );
  $colors_bo=Array("rgba(0,0,0,1)","rgba(240,94,35,1)","rgba(50,255,50,1)", "rgba(180,67,29,1)", "rgba(255,0,0,1)", "rgba(148,0,211,1)", "rgba(119,136,153,1)", "rgba(127,255,212,1)", "rgba(107,142,35,1)", "rgba(255,215,0,1)" );
  $i=0; $count=0;
  echo '<script>
    var config =
    { type: \'line\',
      data:
      {
        labels: ['.'"'.implode('","', $x).'"'.'],
        datasets:
        [ ';
  $n_line= count($y);
  foreach ($y as $dataline)
  { $name=$dataline[0];
    $data=$dataline[1];
    echo '
          { backgroundColor: "'.$colors_bk[$i].'",
            borderColor: "'.$colors_bo[$i].'",
            data: ['.implode(",", array_values($data)).'],
            label: \''.$name.'\' ';

    $i=($i+1)% count($colors_bk); $count++;
    if ( $count<$n_line  ) { echo '          },'; }
    if ( $count>=$n_line ) { echo '          }'; }
  };
  echo '
        ],
      },
      options:
      { steppedLine: true,
         scales: {
                    yAxes: [{
                            display: true
                           }]
                }      
      }
    };
    window.onload = function()
    { var ctx = document.getElementById(\'chart-area\').getContext(\'2d\');
      window.myPie = new Chart(ctx, config);
    };
  </script>';
}

function selettoreCommesse()
{ $sa="selected"; $sb=""; $sc=""; $sd=""; $se=""; $sf=""; $sg=""; $sh=""; 
  if ($_SESSION['Sodl']!="") $sa="";
  if ($_SESSION['Sodl']=="TIM") $sb="selected"; 
  if ($_SESSION['Sodl']=="TIMC") $sc="selected"; 
  if ($_SESSION['Sodl']=="TIMB") $sd="selected"; 
  if ($_SESSION['Sodl']=="EOLO") $se="selected"; 
  if ($_SESSION['Sodl']=="WGR") $sf="selected"; 
  if ($_SESSION['Sodl']=="TELEBIT") $sg="selected"; 
  if ($_SESSION['Sodl']=="VODAFONE") $sh="selected"; 
  echo "<select id=riv onchange=setSession('Sodl',$(this).val());>
      <option $sa value=''>Tutti gli ODL</option>
      <option $sb value='TIM'>Tutto TIM</option>
      <option $sc value='TIMC'>TIM Consumer</option>
      <option $sd value='TIMB'>TIM Business</option>
      <!--option $se value='EOLO'>EOLO</option-->
      <option $sf value='WGR'>WGR</option>
      <!--option $sg value='TELEBIT'>TELEBIT</option-->
      <!--option $sh value='VODAFONE'>VODAFONE</option-->
    </select>\n";
};

function selettoreRivenditori()
{ $sa="selected"; $sb=""; $sc=""; 
  if ($_SESSION['Sreseller']!="") $sa=""; 
  if ($_SESSION['Sreseller']==160) $sb="selected"; 
  if ($_SESSION['Sreseller']==160000) $sc="selected"; 
  echo "<select id=riv onchange=setSession('Sreseller',$(this).val());>
     <option $sa value=0>Tutti i contratti</option>
     <option $sb value=160>BOC</option>
     <option $sc value=160000>BOC OUT</option>\n";
  $q="select id, firma from dealers where visible>0 and 
      id in (select iddealer from wgr_dealer_groups where groupname like 'BOC' or groupname like 'BOC OUT') order by firma";
  $r=perform_query($q); 
  foreach ($r as $l)
  { if (inGroup($l['id'], "RESP BOC")) { $l['firma'].=" (RESP BOC)"; }
    else if (inGroup($l['id'], "BOC OUT")) { $l['firma'].=" (BOC OUT)"; } else $l['firma'].=" (BOC)";   
    $sel=""; if ($_SESSION['Sreseller']==$l['id']) $sel="selected"; 
    echo "<option $sel value=".$l['id'].">".$l['firma']."</option>\n";
  };
  echo "</select>\n";
 };


function creaDettaglio($lista) 
{         //Mi serve il dettaglio
          $det="<div style=text-align:left;>
        <table id=detail>
          <tr>
           <th>#</th>
           <th>Cliente</th>
           <th>Stato</th>
           <th>Contratto</th>
           <th>Data</th>
           <th>Rivenditore</th>
           <th>Gruppo</th>
           <th>Linea</th>
           <th>Offerta</th>
         </tr>";
          $n=1; 
          foreach ($lista as $l)
          { $riv=$l['riv'];
            $res=fastquery("select firma from dealers where id=".$riv); 
            $grp="Rivenditori"; 
            if (inAnyGroup($riv, "BOC")) $grp="BOC"; 
            if (inAnyGroup($riv, "BOC OUT")) $grp="BOC OUT";
            $line=fastquery("select popis from cable_tarif where codicepannello='".$l['line']."' and id!=638 order by id desc");  
            if ($line=="") $line=$l['line']; 
            //Se c'e' lo sconto diventa una welcome
            $cs=fastquery("select codicesconto from wgr_contracts where numero=".$l['id']); 
            if (substr($cs,0,3)=="cli") $cs="amico (".substr($cs,3).")"; 
            if ($cs!="") $off="amico"; 
            if ($cs=='welcome') $off='welcome';
            if ($cs=='cavosteso') $off='cavosteso';
            //Se c'e' inst gratis diventa una gratis 
            $inst=trim(fastquery("select promo2020 from wgr_contracts where numero=".$l['id'])); 
            //if ($inst!="prime") $off="inst.gratis ".$off; 
            if ($l['username']=='') $l['username']=fastquery("select firma_fakt from cable_users where id=".$l['id']); 
            $det.= "<tr>
              <td sorttype=numeric>$n</td>
              <td>".$l['username']."</td>
              <td>".$l['stato']."</td>
              <td sorttypw=numeric>".$l['id']."</td>
              <td sorttype=date>".date("d-m-Y", $l['ins_time'])."</td>
              <td>$res</td>
              <td>$grp</td>
              <td>$line</td>
              <td>$inst $cs</td>
            </tr>"; 
            $n++;
          };
          $det.= "</table></div>"; 
  return $det;
}; 
function creaDettaglioKo($r,$annomese='')
{ $det="<div style=text-align:left;>
        <table id=detail_ko>
          <tr>
            <th>#</th>
            <th>Installatore</th>
            <th>n. ko</th>
            <th>n. completate</th>
            <th>totale</th>
            <th>% ko</th>
            <th style=color:gray;>n. ripianificate</th>
            <th style=color:gray;>% ripianificate</th>
          </tr>"; 
  $cnt=1;
  $total=Array();  
  $numko=Array();  
  foreach ($r as $l)
  { if ($l['Installatore']>0) $inst=fastquery("select firma from dealers where id=".$l['Installatore']); 
    if ($l['Stato']=='completed') { $total[$inst]++;};
    if ($l['Stato']=='ko') { $total[$inst]++; $numko[$inst]++; };
  };
  //qui conto anche le ripianificate, fuori dal totale.
//print_r($total); echo "<br>"; print_r($numko); echo "<br><br><br>"; 
  foreach ($total as $a=>$b)
  { $att=$b - $numko[$a]; 
    $perc=0; if ($att>0) $perc=intval(100*$numko[$a]/($att+$numko[$a])); 
    $numrip=Array(); $totrip=0;
    if ($annomese!='')
    { $anno=substr($annomese,0,4); $mese=substr($annomese,4);  
      $q="select id, numero, installer, causaRipianifica from wgr_storicoInstallatori where substr(data,1,8) like '".$anno."-".$mese."-' and causaRipianifica!='' and type='installazione'";
      $rb=perform_query($q);     
      foreach ($rb as $lb)
      { $instb=fastquery("select firma from dealers where id=".$lb['installer']); 
        $numrip[$instb]++; $totrip++; 
      };
    };
//echo "<textarea>".print_r($numrip,1)."</textarea><br>";

    $percrip=0; if ($totrip>0) $percrip=intval(100*$numrip[$a]/$totrip); 
    $det.="<tr>
            <td sorttype=numeric>$cnt</td>
            <td>$a</td>
            <td sorttype=numeric style=text-align:center;>".intval($numko[$a])."</td>
            <td sorttype=numeric style=text-align:center;>$att</td>
            <td sorttype=numeric style=text-align:center;>".($att+intval($numko[$a]))."</td>  
            <td sorttype=numeric style=text-align:center;>$perc%</td>
            <td sorttype=numeric style=text-align:center;color:gray;>".intval($numrip[$a])."</td>
            <td sorttype=numeric style=text-align:center;color:gray;>$percrip%</td>
          </tr>"; 
    $cnt++; 
  };
  $det.="</table>";
  return $det; 
};
function creaDettaglioInterne($r) //% di inst. interne per tecnico.
{ $det="<div style=text-align:left;>
        <table id=detail_int>
          <tr>
            <th>#</th>
            <th>Installatore</th>
            <th>n. interne</th>
            <th>n. esterne</th>
            <th>n. non specificate</th>
            <th>totale</th>
            <th>% interne</th>
            <th>% non specificate</th>
          </tr>"; 
  $cnt=1;
  $total=Array();  
  $numint=Array();  
  $numns=Array();
  foreach ($r as $l)
  { if ($l['Installatore']>0) $inst=fastquery("select firma from dealers where id=".$l['Installatore']); 
    if ($l['Stato']=='completed') 
    { $total[$inst]++;
      if ($l['Inst']=='IN') $numint[$inst]++; 
      if ($l['Inst']=='')   $numns[$inst]++; 
    };
  };
//print_r($total); echo "<br>"; print_r($numko); echo "<br><br><br>"; 
  foreach ($total as $a=>$b)
  { $att=$b - ($numint[$a] + $numns[$a]); 
    $perc=0; if ($b>0) $perc=intval(100*$numint[$a]/($b)); 
    $perc2=0; if ($b>0) $perc2=intval(100*$numns[$a]/($b)); 
    $det.="<tr>
            <td sorttype=numeric>$cnt</td>
            <td>$a</td>
            <td sorttype=numeric style=text-align:right;padding-right:4em;>".intval($numint[$a])."</td>
            <td sorttype=numeric style=text-align:right;padding-right:4em;>$att</td>
            <td sorttype=numeric style=text-align:right;padding-right:7em;>".intval($numns[$a])."</td>
            <td sorttype=numeric style=text-align:right;padding-right:2em;>$b</td>  
            <td sorttype=numeric style=text-align:right;padding-right:3em;>$perc%</td>
            <td sorttype=numeric style=text-align:right;padding-right:5.5em;>$perc2%</td>
          </tr>"; 
    $cnt++; 
  };
  $det.="</table>";
  return $det; 
};

function creaDettaglioInst($r)
{  $det="<div style=text-align:left;>
        <table id=detail>
          <tr>
           <th>#</th>
           <th>Cliente</th>
           <th>Stato</th>
           <th>Tipo</th>
           <th>Committente</th>
           <th>Data</th>
           <th>Installatore</th>
           <th>In/Outdoor</th>
         </tr>";
  $cnt=1; 
  foreach ($r as $l)
  { $cli=fastquery("select username from cable_users where id=".$l['Id']); 
    $inst="";
    if ($l['Installatore']>0) $inst=fastquery("select firma from dealers where id=".$l['Installatore']); 
    $ind='-';
    if ($l['Inst']!='') $ind=$l['Inst']; 
    $st=$l['Stato']; if ($st=="kor") $st="ko remoto";
    $motivo="";  
    if ($st=="ko")
    { $motivo=fastquery("select motivo from wgr_compensiSAT where idcliente=".$l['Id']); 
    };
    $det.="
        <tr> 
          <td sorttype=numeric>$cnt</td>
          <td sorttype=numeric>".$l['Id']." $cli</td>
          <td>$st $motivo</td>
          <td>".$l['Committente']."</td> 
          <td>".$l['Tipo']."</td>
          <td sorttype=date>".revdate($l['Data'])."</td>
          <td>$inst</td>
          <td>$ind</td>
        </tr>"; 
    $cnt++; 
  };
  $det.="</table>";
  $det=implode('',explode('undefined',$det)); 
  return $det;
};
function obiettivoImpianti($anno,$mese)
{ $q="select obiettivoMese from zwnt_mesiImpiantiNoIVA where annomese='".$anno.$mese."'"; 
  return fastquery($q); 
};


//--------------------------------------------------------------------------//
//---------------------------Codice-principale------------------------------//
//-----------------------Selettore-categoria-e-grafici----------------------//
//--------------------------------------------------------------------------//
_head();
flush(); ob_flush();
//echo "[".$_SESSION['$tech']."]<br>"; 
$sa="selected"; $sb=""; if ($_SESSION['Sbrand']=='INST') { $sa=""; $sb="selected"; }; 
$va=""; $vb=""; 
if ( inGroup($_SESSION['gipUser'], "TECNICI") 
     && !inGroup($_SESSION['gipUser'], "PIANIFICAZIONI")
     && !in_array($_SESSION['gipUser'], Array(62,117)))
{ $va="style=display:none"; $sa=""; $sb="selected"; $_SESSION['Sbrand']='INST'; 
};
if (inGroup($_SESSION['gipUser'], "PIANIFICAZIONI") && !inGroup($_SESSION['gipUser'], "PROJECT MANAGER IMPIANTI")) 
{ $va="style=display:none"; $sa=""; $sb="selected"; $_SESSION['Sbrand']='INST';
};
$BI="<div class=\"brandINST $sb\"$vb onclick=setSession('Sbrand','INST');></div>";
if (!in_array($_SESSION['gipUser'],Array(62,117)) 
       && (inGroup($_SESSION['gipUser'],"BOC") || inGroup($_SESSION['gipUser'],"BOC OUT")) ) $BI="";
echo "
  <table id=paget>
    <tr>
      <td>
        <div class=selettoribrand>
          <div class=\"brandWGR $sa\" $va onclick=setSession('Sbrand','');></div>  $BI       
        </div>
      </td>";
  $sa="selected"; $sb=""; $sc=""; $sd=""; $se=""; $sf=""; $tech="Totale";
  if ($_SESSION['Stech']=='WGR') { $sa=""; $sb="selected"; $tech="WiFi"; };
  if ($_SESSION['Stech']=='LTE') { $sa=""; $sc="selected"; $tech="Qubo indoor"; };
  if ($_SESSION['Stech']=='LTEO') { $sa=""; $sf="selected"; $tech="Qubo 4G Outdoor"; };
  if ($_SESSION['Stech']=='TIM') { $sa=""; $sd="selected"; $tech="TIM"; };
  if ($_SESSION['Stech']=='EOLO') { $sa=""; $se="selected"; $tech="EOLO"; };
if ($_SESSION['Sbrand']=='')
{ //Selettori per la parte wireless
  echo "
      <td class=tech_images>
        <div class=\"$sa\" id=tech_tutto title=Tutto onclick=setSession('Stech','');></div> 
        <div class=\"$sb\" id=tech_WGR  title=Wireless onclick=setSession('Stech','WGR');></div> 
        <div class=\"$sc\" id=tech_LTE  title=\"Qubo indoor\" onclick=setSession('Stech','LTE');></div> 
        <div class=\"$sf\" id=tech_LTEO title=\"Qubo 4G Outdoor\" onclick=setSession('Stech','LTEO');></div> 
        <div class=\"$sd\" id=tech_TIM  title=TIM  onclick=setSession('Stech','TIM');></div> 
        <div class=\"$se\" id=tech_EOLO title=EOLO onclick=setSession('Stech','EOLO');></div> 
      </td>
    "; 
} else if ($_SESSION['Sbrand']=="INST")
{ //Selettori per la parte impianti. Nulla sopra. 
  echo "<td></td>"; 
};
echo "
    </tr>\n";   
//Ora il centro pagina: per prima cosa il menu di sinistra.
echo "
    <tr>
      <td class=sezionePulsanti>"; 
if ($_SESSION['Sbrand']=="")
{ $sa=""; $sb="selected"; $sc=""; $sd=""; $se=""; $sf=""; $sg=""; $sh=""; $si="";
  if ($_SESSION['Scount']=='tkGuasti')   { $sb=""; $sa="selected"; }; 
  if ($_SESSION['Scount']=='att')        { $sb=""; $sc="selected"; }; 
  if ($_SESSION['Scount']=='el')         { $sb=""; $sd="selected"; }; 
  if ($_SESSION['Scount']=='ko')         { $sb=""; $se="selected"; }; 
  if ($_SESSION['Scount']=='dis')        { $sb=""; $sf="selected"; }; 
  if ($_SESSION['Scount']=='geri')       { $sb=""; $sg="selected"; }; 
  if ($_SESSION['Scount']=='incompleti') { $sb=""; $sh="selected"; }; 
  if ($_SESSION['Scount']=='simtim')     { $sb=""; $si="selected"; }; 
  $brandObiettivi="WGR";
  if ($_SESSION['Stech']=="LTE" || $_SESSION['Stech']=="LTEO") $brandObiettivi="LTE";
  echo "
        <div class=pulsantisx>
          <div class=\"$sa\" onclick=setSession('Scount','tkGuasti');>Ticket Guasti</div>
          <div class=\"$sb\" onclick=setSession('Scount','');>Inseriti</div>
          <div class=\"$sc\" onclick=setSession('Scount','att');>Report $tech</div>
          <div onclick=\"window.open('cambiLinea.php?obiettivi&_actualBrand=$brandObiettivi','_blank');\">Obiettivi</div>
          <div onclick=\"window.open('test_nonpagantiB.php?tab=2','_blank');\">Durata cliente</div>
          <!--<div class=\"$sd\" onclick=setSession('Scount','el');>Eliminati</div>-->
          <!--<div class=\"$se\" onclick=setSession('Scount','ko');>KO installazioni</div>-->
          <!--<div class=\"$sf\" onclick=setSession('Scount','dis');>Disdette</div>-->
          <!--<div class=\"$sg\" onclick=setSession('Scount','geri');>Rec.Crediti</div>-->
          <!--<div class=\"$sh\" onclick=setSession('Scount','incompleti');>Incompleti</div>-->
          <div class=\"$si\" onclick=setSession('Scount','simtim'); style=color:white; >SIM ATTIVATE</div>";
  selettoreRivenditori();
  //Lascio la sezione per i totali da valorizzare, esattamente come ora. 
} else if ($_SESSION['Sbrand']=="INST")
{ echo "
        <div class=pulsantisx>
          <div class=$sb onclick=setSession('Scount','');>Inseriti</div>";
  selettoreCommesse();
};
  echo "
        </div note=pulsantisx>";
  
if ($_SESSION['Sbrand']!="INST")
{ echo "
        <div class=totali>
          <div class=titolototali>inseriti  totali</div>      
          <div class=totaleanno>
            <p> Da 10/2024 a 09/2025</p><div id=ACORR></div> 
          </div>
          <div class=totaleanno>
            <p>12 mesi prec.</p><div id=ASCO></div>
          </div>  
          <div class=totaleanno>
            <p>Incremento%</p><div id=INCRE></div>
          </div>  
        </div note=totali>";
echo "
      </td>"; 
};
echo "
      </td>
      <td class=SezioneGrafico>"; 
if ($_SESSION['Sbrand']=='' && canEditCostiLTEQ())
{ echo "<div class=btn_dettaglio style=\"width:12em;margin-bottom:1em;\" onclick=openCostiLTEQ();>Costi Qubo</div>";
};

//Qui manca il bottone ANNO, da implementare per tutto WGR tranne i tickets. 
  $sa=""; $sb=""; $sc=""; $sd="selected";
  if ($_SESSION['Sanno']!="" && intval($_SESSION['Sanno'])!= (date("Ym")-100))
  { if (intval($_SESSION['Sanno'])== intval((date("Y")-2)."01")) { $sa="selected"; $sd=""; }; 
    if (intval($_SESSION['Sanno'])== intval((date("Y")-1)."01")) { $sb="selected"; $sd=""; }; 
    if (intval($_SESSION['Sanno'])== intval(date("Y")."01"))     { $sc="selected"; $sd=""; }; 
  };
  if (($_SESSION['Sbrand']=='') && $_SESSION['Scount']!='tkGuasti' && $_SESSION['Scount']!='simtim') 
  echo "
    <div class=bottoniAnno>
      <div onclick=setSession('Sanno','".(date("Y")-2)."01'); class=$sa >".(date("Y")-2)."</div>
      <div onclick=setSession('Sanno','".(date("Y")-1)."01'); class=$sb >".(date("Y")-1)."</div>
      <div onclick=setSession('Sanno','".(date("Y"))."01');   class=$sc >".(date("Y"))."</div>
      <div onclick=setSession('Sanno','".(date("Ym")-100)."'); class=$sd >Ultimi 12 mesi</div>
    </div>\n"; 
  if (($_SESSION['Sbrand']=='') && $_SESSION['Scount']!='tkGuasti' && $_SESSION['Scount']!='simtim' && $_SESSION['Scount']!='att')
  { $sa="selected"; $sb=""; 
    if ($_SESSION['Sinterval']=='S') { $sa=""; $sb="selected"; }; 
    echo "
      <select id=interval onchange=setSession('Sinterval',$(this).val()); >
        <option $sa value=\"\">Mensile</option>
        <option $sb value=\"S\">Settimanale</option>
      </select><br>"; 
  } else $_SESSION['Sinterval']='';  
  ob_flush(); flush(); 

//--------------------------------------------------------------------------//
//---------------------------Sezione-ricerca-dati---------------------------//
//---------------Qui-caching-e-comunicazione-con-le-librerie----------------//
//--------------------------------------------------------------------------//
if ($_SESSION['Sbrand']=='')
{ if ($_SESSION['Scount']=='tkGuasti') //--------------TICKET GUASTI PER WGR//
  { echo "<div class=selettoriTickets>"; 
    $sa=""; $sb=""; $sc="selected"; 
    if ($_SESSION['Svis']=='g') { $sa="selected"; $sc=""; };
    if ($_SESSION['Svis']=='s') { $sb="selected"; $sc=""; };
    echo "Visualizzazione 
      <select class=Statselect name=vis onchange=setSession('Svis',\$(this).val());>
        <option $sa value=g>giornaliera</option>
        <option $sb value=s>settimanale</option>
        <option $sc value=\"\">mensile</option>
      </select>&nbsp;&nbsp;";
    $sa="selected"; $sb=""; $sc=""; 
    if ($_SESSION['Stipo']=='g') { $sb="selected"; $sa=""; };
    if ($_SESSION['Stipo']=='n') { $sc="selected"; $sa=""; };
    echo "Tipologia 
      <select name=tipo onchange=setSession('Stipo',\$(this).val());>
        <option $sa value=\"\">tutti</option>
        <option $sb value=g>guasti</option>
        <option $sc value=n>rallentamenti</option>
      </select>&nbsp;&nbsp;"; 

    //Operatori: direi gruppo noc e helpdesk.. 
    $q="select id, firma from dealers where visible>0 and id in (select iddealer from wgr_dealer_groups where groupname like 'coord tecnico' or groupname like 'helpdesk')";
    $r=perform_query($q);
    $a="selected"; if (intval($_SESSION['Sop'])!=0) $sa="";   
    echo "Operatori
      <select name=op onchange=setSession('Sop',\$(this).val());>
        <option $sa value=0>-tutti-</option>";
    foreach ($r as $l) 
    { $s=""; if ($l['id']==intval($_SESSION['Sop'])) $s="selected"; 
     echo "<option $s value=".$l['id'].">".$l['firma']."</option>"; 
    };
    echo "
      </select>&nbsp;&nbsp;";
    $sa="selected"; $sb=""; $sc="";
    if ($_SESSION['Stpt']==-2) { $sa=""; $sb="selected"; };
    if ($_SESSION['Stpt']==-3) { $sa=""; $sc="selected"; };
    echo "Selezione 
      <select name=tpt onchange=setSession('Stpt',\$(this).val());>
        <option $sa value=0>-tutti-</option>
        <option $sb value=-2>-tutti tranne problemi cliente-</option>
        <option $sc value=-3>-problemi a carico del cliente-</option>
      </select><br>"; 
    //<input type="hidden" name="m" value="cli">
    //<input type="hidden" name="mode" value="tkGuasti">
    //<input type="hidden" name="exp" value="0">
    echo guastiSezDx();
  } else 
  { //Un po di lavoro comune a tutti i grafici: variabili standards, ad esempio operatori.
    $graph=Array(); 
    $USEDBRAND=$_SESSION['Sbrand']; if ($USEDBRAND=='') $USEDBRAND='WGR';
    $USEDTECH=$_SESSION['Stech']; if ($USEDTECH=='') $USEDBRAND='WGR';
    
    $res=$_SESSION['Sreseller']; 
    $op=Array($res);
    if ($res==160000 || $res==160160)
    { if ($res==160000) $op=Array(); 
      $q="select id from dealers where visible>0 and id in (select iddealer from wgr_dealer_groups where groupname like 'BOC' or groupname like 'BOC ENERGY')";
      $r=perform_query($q);  
      foreach ($r as $l) $op[]=$l['id'];
    }; 
    $numops=count($op); 
    $am=$_SESSION['Sanno']; if ($am=="") $am=date("Ym") - 100; 
    $y=substr($am,0,4); $m=substr($am,4,2); 
    $dal=$y."-".$m."-01"; 
    if ($_SESSION['Sinterval']=='S') 
    { $dal=date(time()-24*3600*7*12); 
      while(date("w",$dal)!=1) $dal=$dal - 24*3600; 
      $dal=date("Y-m-d", $dal); 
    };
    if ($_SESSION['Scount']=='att') //Pulsante "Report:" tuttil attivi, ko, eliminati, disdette, rec. crediti, spediti, incompleti
    { //Grafico mensile e poi settimanale. 
      //Qui ci vuole il ciclo..
      for ($i=0; $i<12; $i++)
      { //Valorizzo il mese
        $anno=$y; 
        $mese=$m + $i + $m_start;   
        if ($mese>12) { $mese=$mese-12; $anno=$y+1; }; 
        if (strlen($mese)<2) $mese='0'.$mese; 
        $numweek=date("W")-(12-$i); $annoweek=date("Y"); 
        if ($numweek<0) 
        { $annoweek--; $numweek=52+$numweek;  
        };    
        if ($_SESSION['Sinterval']=='S') 
        { echo "<div class=ascomparsa>calcolo settimana $numweek $annoweek</div>"; ob_flush(); flush();
          $dt=totaliSettimana($numweek,$annoweek); 
        } else 
        { echo "<div class=ascomparsa>calcolo mese $mese $anno</div>"; ob_flush(); flush();
          $dt=totaliMese($mese,$anno);
          $lista=filtroUsersBrand($dt, $USEDBRAND, $USEDTECH);
          $numtl=contaPerRivenditore($lista, $_session['Sreseller'], $USEDBRAND);
//echo "<textarea>";
          //Ok, cominicamo a separare gli stati. 
          $a=$graph["Tutti (inserimenti del mese)"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=$numtl; $graph["Tutti (inseriti del mese)"]=$a;
          //Filtriamo sulla lista secondo gli stati. Sono esclude disdette e rec.crediti che dovremo aggiungere manualmente.
          $attivi=Array(); $ko=Array(); $eliminati=Array(); $spediti=Array(); $sospesi_ut=Array(); $programmati=Array(); $inseriti=Array();
          $lb=$lista; $lista=Array(); 
          foreach ($lb as $l)
          { $vis=fastquery("select visible from cable_users where id=".$l['id']);
            $at=fastquery("select active_time from cable_users where id=".$l['id']);
            if ($vis==0) 
            { $eliminati[]=$l; $stato="Eliminato"; $l['stato']="Eliminato"; 
            } else
            { $sk=fastquery("select skupina from cable_users where id=".$l['id']);
              if ($sk!=54 && $sk!=58)
              { $sta=fastquery("select name from cable_users_groups where id=$sk"); 
//echo $l['id']." ".$sk." ".$sta."\n"; 
                if ($sk==53 || $sk==43 || $sk==44 || $sk==33) 
                { $sospesi[]=$l; $stato="sospeso"; 
                } else if ($sk==62) 
                { $spediti[]=$l; $stato="Spedito"; 
                } else if (in_array($sk, Array(52,22)) || ($sk==24 && $at==0))  //dismesso mai attivato=ko.
                { $ko[]=$l; $stato="Ko"; 
                } else if ($at>0) 
                { $attivi[]=$l; $stato="Attivo"; 
                } else if ($sk==49)
                { $stato="inserito";
                  $inseriti[]=$l;
                } else if ($sk==50 || $sk==51) //Dipende dalla /89 e per il 51 dall'orario di attivazione
                { $vs=fastquery("select max(vs) from billing_faktury where klientid=".$l['id']);
                  if (substr($vs,0,2)=='89')
                  { $stato="sospeso_ut"; 
                    $sospesi_ut[]=$l; 
                  } else 
                  { $stato="programmato";
                    $programmati[]=$l; 
                  };  
                } else if ($sk==46)
                { $stato="Recupero Crediti";
                } else echo "Stato non gestito: $sk<br>"; 
                $l['stato']=$stato; 
              };
            };  
            $lista[]=$l; 
          }; 
//echo "</textarea>"; 
          $a=$graph["Eliminati"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=count($eliminati); $graph["Eliminati"]=$a;
          $a=$graph["Ko"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=count($ko); $graph["Ko"]=$a;
          $a=$graph["Sospesi"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=count($sospesi); $graph["Sospesi"]=$a;
          $a=$graph["Sospesi UT"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=count($sospesi_ut); $graph["Sospesi UT"]=$a;
          $a=$graph["Inseriti"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=count($inseriti); $graph["Inseriti"]=$a;
          $a=$graph["Programmati"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=count($programmati); $graph["Programmati"]=$a;
          $a=$graph["Spediti"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=count($spediti); $graph["Spediti"]=$a;
          $a=$graph["Attivi"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=count($attivi); $graph["Attivi"]=$a;
          $det=creaDettaglio($lista); 
          $a=$graph["detail"];
          if (!is_array($a)) $a=Array(); 
          $a[$i]=$det;  $graph['detail']=$a; 
          //Finalmente ho tutto. mostro il grafico.
        };
      };
      $attivi=$graph['Attivi']; $persi=Array(); $bilancio=Array(); $dettBilancio=Array();
      preparaGrafico("Report <b>$tech</b> ", $graph, $dal, ($_SESSION['Sinterval']=='S'));
      //Metto sotto un SECONDO grafico con disdette e geri.
      $graph=""; 
      for ($i=0; $i<12; $i++)
      { //Valorizzo il mese
        $anno=$y; 
        $mese=$m + $i + $m_start;   
        if ($mese>12) { $mese=$mese-12; $anno=$y+1; }; 
        if (strlen($mese)<2) $mese='0'.$mese; 
        $listaa=filtroUsersBrand(geriMese($mese,$anno), $USEDBRAND, $USEDTECH); 
        $listab=filtroUsersBrand(disdetteMese($mese,$anno), $USEDBRAND, $USEDTECH);
        $lista=Array(); 
        foreach ($listaa as $l)        
        { $l['stato']="Recupero Crediti"; 
          $lista[]=$l; 
        };
        foreach ($listab as $l)        
        { $l['stato']="Disdetta"; 
          $lista[]=$l; 
        };
        $a=$graph["Rec.Crediti"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=count($listaa); $graph["Rec.Crediti"]=$a;
        $a=$graph["Disdette"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=count($listab); $graph["Disdette"]=$a;
        $det=creaDettaglio($lista); 
        $a=$graph["detail"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=$det;  $graph['detail']=$a;    
        $persi[$i]=count($lista); 
        $bilancio[$i]=$attivi[$i] - $persi[$i]; 
        if ($_SESSION['Sinterval']!='S')
        { $bl=bilancioLTEQMese($mese,$anno);
          $dettBilancio[$i]=$bl[1];
        };
      };
      echo "<br><br><hr>"; 
      preparaGrafico("Disdette/Rec.crediti <b>$tech</b> ", $graph, $dal, ($_SESSION['Sinterval']=='S'));
     
      $graph=Array("Attivati"=>$attivi, "Persi"=>$persi, "Bilancio"=>$bilancio/*, "-"=>Array(0,0,0,0,0,0,0,0,0,0,0,0)*/); 
      if ($_SESSION['Sinterval']!='S')
      { $graph['detail']=$dettBilancio;
        $graph['detailName']="Bilancio";
      };
      echo "<br><br><hr>"; 
      preparaGrafico("Bilancio Totale <b>$tech</b> ", $graph, $dal, ($_SESSION['Sinterval']=='S'));
      avviaGrafici();
    };
    if ($_SESSION['Scount']=='simtim')
    { $graph=Array(); 
      for ($i=0; $i<12; $i++)
      { //Valorizzo il mese
        $anno=$y; 
        $mese=$m + $i;   
        if ($mese>12) { $mese=$mese-12; $anno=$y+1; }; 
        if (strlen($mese)<2) $mese='0'.$mese; 
        $r=simTimTotali($mese,$anno);
        $a=$graph["SIM Attive (10 euro)"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=count($r[1]); $graph["SIM Attive (10 euro)"]=$a; 
        $a=$graph["SIM Dormienti (2.5 euro)"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=count($r[0]); $graph["SIM Dormienti (2.5 euro)"]=$a; 
      };
      preparaGrafico("SIm TIM", $graph, $dal, false);
      avviaGrafici();
    };

    if ($_SESSION['Scount']=='') //GESTIONE LEADS COMMERCIALI
    { //Grafico mensile e poi settimanale. 
      //Qui ci vuole il ciclo..
      for ($i=0; $i<12; $i++)
      { //Valorizzo il mese
        $anno=$y; 
        $mese=$m + $i;   
        if ($mese>12) { $mese=$mese-12; $anno=$y+1; }; 
        if (strlen($mese)<2) $mese='0'.$mese; 
        $numweek=date("W")-(12-$i); $annoweek=date("Y"); 
        if ($numweek<0) 
        { $annoweek--; $numweek=52+$numweek;  
        };    
        if ($_SESSION['Sinterval']=='S') 
        { echo "<div class=ascomparsa>calcolo settimana $numweek $annoweek</div>"; ob_flush(); flush();
          $dt=totaliSettimana($numweek,$annoweek); 
        } else 
        { echo "<div class=ascomparsa>calcolo mese $mese $anno</div>"; ob_flush(); flush();
          $dt=totaliMese($mese,$anno);
        };
        $lista=filtroUsersBrand($dt, $USEDBRAND, $USEDTECH);
        $numtl=contaPerRivenditore($lista, $_SESSION['Sreseller'], $USEDBRAND);
//echo "numtl vale contaPerRivenditore($lista, ".$_SESSION['Sreseller'].", $USEDBRAND) = $numtl<br>"; 
        if ($_SESSION['Sinterval']=='S')
        { $leads=leadsSettimanaOperatori('WGR', $op, $numweek,$annoweek);
        } else $leads=leadsMeseOperatori('WGR', $op, $mese,$anno);  
        $days=Array(31,28,31,30,31,30,31,31,30,31,30,31); 
        $settimanaMese=$days[$mese-1]/7; 
        global $SogliaLeadsConnect, $SogliaInseritiConnect;
        global $SogliaLeadsEnergy, $SogliaInseritiEnergy;
        global $SogliaLeadsEnergy2, $SogliaInseritiEnergy2;
        if ($_SESSION['Sinterval']=='S') $settimanaMese=1; 
        $soglialeads=round($SogliaLeadsConnect * $settimanaMese, 1);
        $sogliainseriti=round($SogliaInseritiConnect * $settimanaMese, 1);
        $a=$graph["Inseriti"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=$numtl; $graph["Inseriti"]=$a; //Devo valorizzare numtl  

        $a=$graph["Leads"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=$leads; $graph["Leads"]=$a; 

        $a=$graph["Soglia Leads"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=$soglialeads*$numops; $graph["Soglia Leads"]=$a; 

        $a=$graph["Soglia Inseriti"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=$sogliainseriti*$numops; $graph["Soglia Inseriti"]=$a; 
      
        $det.= creaDettaglio($lista); 
        $a=$graph["detail"];
        if (!is_array($a)) $a=Array(); 
        $a[$i]=$det;  $graph['detail']=$a; 
      };
      //Finalmente ho tutto. mostro il grafico.
      preparaGrafico("Situazione <b>gen.connect</b> ", $graph, $dal, ($_SESSION['Sinterval']=='S'));
      avviaGrafici();
    };
  };
};
if ($_SESSION['Sbrand']=='INST')
{ //Per prima cosa il riquadro con lo stato del buffer, come su report squadre.
  $r=bufferODL();
  $comm=$_SESSION['Sodl'];
  $buf=count($r); //Questo da calcolare.
  $nt=count(listaTecniciAttiviSettimana(date("Y-m-d"))); 
  $older=date("Y-m-d", time()+24*3600); $olderid=-1; 

  $buf=0; 
  foreach ($r as $l)
  { if (($l['Committente']==$comm || $comm=='') && $l['Data']<$older){ $olderid=$l['Id']; $olderTech=$l['Committente']; $older=$l['Data']; }; 
    if ($l['Committente']==$comm || $comm=='') $buf++; 
  };
  $col="green"; if ($buf>$nt*50) $col="red"; 

  $dtolder=revdate($older); 
  $infoolder=fastquery("select concat(id,' ',username) from cable_users where id=$olderid")." ".$olderTech; 
  if ($olderid==-1) { $dtolder='-'; $infoolder="Nessuna installazione prevista"; };  
  $numokInt=0; $numokInst=0; $numokRes=0;         //Installazioni ok.
  $numkoInt=0; $numkoInst=0; $numkoRes=0;         //Installazioni ko.
  $numkor=0; $numguastiokInt=0;                   //KO da remoto e guasti ok.  
  $r=listaODLNelMese(date("Ym", time()-24*3600*0)); 
  foreach ($r as $l)
  { //Devo capire se è un interno, un installatore o un rivenditore.
    if ($l['Committente']==$comm  || ($comm=="TIM" &&substr($l['Committente'],0,3)==$comm)  || $comm=='')
    { if (inGroup($l['Installatore'],'TECNICI') ||  inGroup($l['Installatore'],'PIANIFICAZIONI') || $l['Installatore']==297)
      { if ($l['Stato']=="completed") 
        { if ($l['Tipo']=='Installazione') { $numokInt++; } else $numguastiokInt++; 
        } else if ($l['Stato']=="kor") $numkor++; else $numkoInt++; 
      } else if (isOnlyInstaller($l['Installatore'])) 
      { if ($l['Stato']=="completed") 
        { if ($l['Tipo']=='Installazione') { $numokInst++; } else $numguastiokInt++; 
        } else if ($l['Stato']=="kor") $numkor++; else $numkoInst++;
      } else 
      { if ($l['Stato']=="completed") 
        {if ($l['Tipo']=='Installazione') { $numokRes++; } else $numguastiokInt++;  
        } else if ($l['Stato']=="kor") $numkor++; else $numkoRes++;
      };
    }; 
  };
  $numok = $numokInt + $numokInst + $numokRes; 
  $numko = $numkoInt + $numkoInst + $numkoRes; 
  $mesi=Array("","Gennaio","Febbraio","Marzo","Aprile","Maggio","Giugno","Luglio","Agosto","Settembre","Ottobre","Novembre","Dicembre");
  $mese=$mesi[intval(date("m", time()-24*3600*0))]; 
  echo "
  <div class=tbc>
    <table class=tableBuffer>
      <tr>
        <td>&nbsp;</td>
        <td>Buffer $comm (da fare)</td>
        <td style=color:$col;>$buf</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td>&nbsp;</td>
        <td>Data inst.più vecchia</td>
        <td style=cursor:pointer; title=\"$infoolder\">$dtolder</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td>$mese</td>
        <th>Inst interni</th>
        <th>Guasti</th>
        <th>Partner inst</th>
        <th>Partner comm+inst</th>
        <td>&nbsp;</td>
      </tr>
      <tr>
        <td>Installazioni OK</td>
        <td class=tborded>$numokInt</td>
        <td class=tborded>$numguastiokInt</td>
        <td class=tborded>$numokInst</td>
        <td class=tborded>$numokRes</td>
        <td>&nbsp;</td>
      </tr>  
      <tr>
        <td>KO sul posto</td>
        <td class=tborded>$numkoInt</td>
        <td class=tborded>-</td>
        <td class=tborded>$numkoInst</td>
        <td class=tborded>$numkoRes</td>
        <td>&nbsp;</td>
      </tr>  
      <tr>
        <td>KO da remoto</td>
        <td class=tborded>$numkor</td>
        <td class=tborded>-</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>  
      <tr>
        <td>TOTALE PRATICHE</td>
        <td class=tborded>".($numok + $numko + $numkor + $numguastiokInt)."</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>  
    </table>
  </div>";
  //Ora lo stesso giro negli ultimi N mesi, in modo da mostrare il grafico
  $graph=Array(); 
  $ginst=Array(); 
  $grec=Array(); 
  $am=date("Ym",time()-24*3600*0) - 100;
 
  $y=substr($am,0,4); $m=substr($am,4,2); 
  $m++; if ($m>12) { $m=$m-12; $y++; }; 
  if ($m<10) $m='0'.$m; 
  $dal=$y."-".$m."-01"; 
  for ($i=0; $i<12; $i++)
  { //Valorizzo il mese
    $anno=$y; 
    $mese=$m + $i;   
    if ($mese>12) { $mese=$mese-12; $anno=$y+1; }; 
    if (strlen($mese)<2) $mese='0'.$mese; 
    $r=listaODLNelMese($anno.$mese); 
    //Filtro per eventuale committente o tutti.
    $rb=$r; $r=Array(); 
    $numko=0; $numkor=0; $numok=0;
    $numrec=0; $numrecko=0; $numreckor=0;
    $numns=0; $numin=0; $numout=0;  
    $numg=0;  
    $rinst=Array(); 
    $idko=Array();
    $idkor=Array();
    foreach ($rb as $l)
    { //echo $l['id']." ".$l['Committente']."/".$comm."<br>"; 
      if ($l['Committente']==$comm  || ($comm=="TIM" &&substr($l['Committente'],0,3)==$comm)  || $comm=='')
      { $r[]=$l;
        if ($l['Stato']=='completed' && $l['Tipo']=='Installazione')
        { $rinst[]=$l;
          if ($l['Inst']=='IN') 
          { $numin++;     
          } else if ($l['Inst']=='OUT') 
          { $numout++; 
          } else $numns++; 
        };
        if ($l['Stato']=="completed") 
        { if ($l['Tipo']=='Installazione') $numok++; else $numg++;    
        }; 
        if ($l['Stato']=="ko") { $numko++; $idko[]=intval($l['Id']); }
        if ($l['Stato']=="kor") { $numkor++; $idkor[]=intval($l['Id']); }
      }; 
    };
    $allidko=array_merge($idko, $idkor);
    if (count($allidko)>0)
    { mysql_query("create table if not exists wgr_koRecuperati (id int auto_increment primary key, numero int not null default 0)");
      $recuperati=perform_query("select numero from wgr_koRecuperati where numero in (".implode(",", $allidko).")");
      foreach ($recuperati as $rr)
      { if (in_array(intval($rr['numero']), $idko)) $numrecko++;
        if (in_array(intval($rr['numero']), $idkor)) $numreckor++;
      };
      $numrec=$numrecko + $numreckor;
    };
    //Comincio a costruire il grafico.
    $pratiche=count($r); //Questi sono i totali.
    $a=$graph["Pratiche"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$pratiche; $graph["Pratiche"]=$a;
    $a=$graph["OK"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$numok; $graph["OK"]=$a;
    $a=$graph["ko (sul posto)"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$numko; $graph["ko (sul posto)"]=$a;
    $a=$graph["ko (da remoto)"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$numkor; $graph["ko (da remoto)"]=$a;
    $a=$graph["Guasti OK"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$numg; $graph["Guasti OK"]=$a;
    $totinst=$numin + $numout + $numns;
    $a=$graph["Soglia ko"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=intval($totinst)/10; $graph["Soglia ko"]=$a;
    
    //Solo se scelgo "tutto": 
    if ($comm=='') 
    { $a=$graph['Obiettivo'];
      if (!is_array($a)) $a=Array(); 
      $a[$i]=obiettivoImpianti($anno,$mese);
      $graph['Obiettivo']=$a; 
    };

    $det="<b style=font-size:150%;>Dettaglio mese ".$mesi[intval($mese)]." ".$anno."</b><br><br>"; 
    $det.= creaDettaglioInst($r); 

    $a=$graph["detail"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$det;  $graph['detail']=$a; 
    $det_ko="<b style=font-size:150%;>% Ko mese ".$mesi[intval($mese)]." ".$anno."</b><br><br>"; 
    $det_ko.= creaDettaglioKo($r,$anno.$mese); 
    $a=$graph["detail_ko"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$det_ko;  $graph['detail_ko']=$a; 

    $a=$ginst['Indoor'];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$numin; $ginst["Indoor"]=$a;
    $a=$ginst['Outdoor'];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$numout; $ginst["Outdoor"]=$a;
    $a=$ginst['Non specificate'];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$numns; $ginst["Non specificate"]=$a;

    //Totale e soglia indoor
    $a=$ginst['Totali'];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$totinst; $ginst["Totali"]=$a;
    $a=$ginst['Soglia indoor'];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=intval(($numin + $numout + $numns)*20/100); $ginst["Soglia indoor"]=$a;

    $a=$ginst["detail"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$det;  $ginst['detail']=$a; 
    $det_int="<b style=font-size:150%;>% Interne ".$mesi[intval($mese)]." ".$anno."</b><br><br>"; 
    $det_int.= creaDettaglioInterne($rinst); 
    $a=$ginst["detail_int"];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$det_int;  $ginst['detail_int']=$a; 

    if ($comm!="WGR")
    { $a=$grec["ko da remoto"];
      if (!is_array($a)) $a=Array();
      $a[$i]=$numkor; $grec["ko da remoto"]=$a;
      $a=$grec["ko sul posto"];
      if (!is_array($a)) $a=Array();
      $a[$i]=$numko; $grec["ko sul posto"]=$a;
      $a=$grec["recuperati da remoto"];
      if (!is_array($a)) $a=Array();
      $a[$i]=$numreckor; $grec["recuperati da remoto"]=$a;
      $a=$grec["recuperati sul posto"];
      if (!is_array($a)) $a=Array();
      $a[$i]=$numrecko; $grec["recuperati sul posto"]=$a;
      $a=$grec["ko totali"];
      if (!is_array($a)) $a=Array();
      $a[$i]=$numko + $numkor; $grec["ko totali"]=$a;
      $a=$grec["recuperati totali"];
      if (!is_array($a)) $a=Array();
      $a[$i]=$numrec; $grec["recuperati totali"]=$a;
    };
  };
  $s=$graph["Soglia ko"]; unset($graph["Soglia ko"]);
  $graph=array_merge(Array("Soglia ko"=>$s), $graph);
  preparaGrafico("Situazione <b>gen.impianti</b>", $graph, $dal, false);

  //Faccio un secondo grafico con indoor, outdoor, non specificato.
  if (substr($comm,0,3)=='TIM')
  { echo "<br><br><br>"; 
    $s=$ginst["Soglia indoor"]; unset($ginst["Soglia indoor"]);
    $ginst=array_merge(Array("Soglia indoor"=>$s), $ginst);
    preparaGrafico("Installazioni indoor/outdoor", $ginst, $dal, false);
  };

/////////////////////////////////////////////////////////

  //Qui un terzo grafico con le cause di ripianificazione 
  $grip=Array(); 
  $y=substr($am,0,4); $m=substr($am,4,2); 
  $m++; if ($m>12) { $m=$m-12; $y++; }; 
  if ($m<10) $m='0'.$m; 
  $dal=$y."-".$m."-01"; 
  for ($i=0; $i<12; $i++)
  { //Valorizzo il mese
    $anno=$y; 
    $mese=$m + $i;   
    if ($mese>12) { $mese=$mese-12; $anno=$y+1; }; 
    if (strlen($mese)<2) $mese='0'.$mese; 
    $q="select id, numero, causaRipianifica from wgr_storicoInstallatori where substr(data,1,8) like '".$anno."-".$mese."-%' and causaRipianifica!='' and type='installazione'";
//echo $q."<br>"; 
    $r=perform_query($q); 
    $cause=Array( Array("piove","Maltempo",0),
                  Array("ritardo","Tecnico arrivato in ritardo",0),
                  Array("cli","Il cliente non si fa trovare",0) ,
                  Array("imprevisto","Imprevisto cliente",0) ,
                  Array("tech","Imprevisto tecnico",0), 
                  Array("scal","Cliente da rifare con scalone",0),
                  Array("due","Necessità di due tecnici",0),
                  Array("comm","Motivi commerciali",0), 
                );  
    foreach ($r as $l)
    { //Vedo di farle collimare con il filtro $COMM 
      //$cr=$cause[$l['causaRipianifica']]; 
      $c=committenteOdl($l['numero']);
//echo "Comm ".$l['numero'].": [$c/$comm]<br>";
      if ($c==$comm || $comm=='' || ($comm=="TIM" && substr($c,0,3)==$comm)) //Dato interessante, rientra nel filtro.  
      { $cb=Array();
        foreach ($cause as $cs)
        { if ($cs[0]==$l['causaRipianifica']) $cs[2]++; 
          $cb[]=$cs; 
        };
        $cause=$cb; 
      };
    };
    //Le ho contate. Posso aggiungere al grafico.   
    $totrip=0; 
    foreach ($cause as $c)
    { $a=$grip[$c[1]]; 
      if (!is_array($a)) $a=Array(); 
      $a[$i]=$c[2];
//if ($anno.$mese=='202511') echo "Imposto ".$c[1]." a ".$c[2]."<br>"; 
      $grip[$c[1]]=$a; 
      $totrip+=$c[2]; 
    };
    //Totale e soglia ripianificazioni
    $totali=$graph["OK"]; 
    $a=$grip['Totali'];
    if (!is_array($a)) $a=Array(); 
    $a[$i]=$totrip; $grip["Totali"]=$a;
    $a=$grip['Soglia ripianificati'];
    if (!is_array($a)) $a=Array(); 
    $it=$totali[$i];
    $a[$i]=(intval($it))/10; $grip["Soglia ripianificati"]=$a;
  }; 
  echo "<br><br><br>"; 
  $s=$grip["Soglia ripianificati"]; unset($grip["Soglia ripianificati"]);
  $grip=array_merge(Array("Soglia ripianificati"=>$s), $grip);
  preparaGrafico("Cause di ripianificazione", $grip, $dal, false);

//////////////////////////////////////////////////////
  //Qui un terzo grafico con le cause di KO
  $grip=Array(); 
  $y=substr($am,0,4); $m=substr($am,4,2); 
  $m++; if ($m>12) { $m=$m-12; $y++; }; 
  if ($m<10) $m='0'.$m; 
  $dal=$y."-".$m."-01"; 
  for ($i=0; $i<12; $i++)
  { //Valorizzo il mese
    $anno=$y; 
    $mese=$m + $i;   
    if ($mese>12) { $mese=$mese-12; $anno=$y+1; }; 
    if (strlen($mese)<2) $mese='0'.$mese; 
    $q="select id, idcliente, motivo, idcliente as numero from wgr_compensiSAT where idcliente in (select numero from wgr_storicoInstallatori where substr(data,1,8) like '".$anno."-".$mese."-%' and stato='ko' and type='installazione')";
    $r=perform_query($q); 
    $cause=Array( Array("Irreperibile con intervento","Irreperibile con intervento",0),
                  Array("Motivi commerciali","Motivi commerciali",0),
                  Array("Non vuole lavori in casa","Non vuole lavori in casa",0) ,
                  Array("Prestazioni servizio non adeguate (Lentezza)","Prestazioni servizio non adeguate (Lentezza)",0) ,
                  Array("Indirizzo errato","Indirizzo errato",0), 
                  Array("Infrastruttura cliente","Infrastruttura cliente",0),
                  Array("Indirizzo non coperto dal servizio richiesto (Zero segnale)","Indirizzo non coperto dal servizio richiesto (Zero segnale)",0)
                );  
    foreach ($r as $l)
    { //Vedo di farle collimare con il filtro $COMM 
      //$cr=$cause[$l['causaRipianifica']]; 
      $l['motivo']=implode('', explode('undefined', $l['motivo'])); 
      $c=committenteOdl($l['numero']);
//echo "Comm ".$l['numero'].": [$c/$comm]<br>";
      if ($c==$comm || $comm=='' || ($comm=="TIM" && substr($c,0,3)==$comm)) //Dato interessante, rientra nel filtro.  
      { $cb=Array();
        foreach ($cause as $cs)
        { if ($cs[0]==$l['motivo']) $cs[2]++; 
          $cb[]=$cs; 
        };
        $cause=$cb; 
      };
    };
    //Le ho contate. Posso aggiungere al grafico.   
    foreach ($cause as $c)
    { $a=$grip[$c[1]]; 
      if (!is_array($a)) $a=Array(); 
      $a[$i]=$c[2];
//if ($anno.$mese=='202511') echo "Imposto ".$c[1]." a ".$c[2]."<br>"; 
      $grip[$c[1]]=$a;  
    };
  }; 
  echo "<br><br><br>"; 
  
  preparaGrafico("Cause di ko", $grip, $dal, false);

  if ($comm!="WGR")
  { echo "<br><br><br>";
    preparaGrafico("KO e recuperati", $grec, $dal, false);
  };

  avviaGrafici();
};
echo "
      </td>
    </tr>
</table>"; 
flush(); ob_flush();
tail();
//----------------------------------//
