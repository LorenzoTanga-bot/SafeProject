
<h3  align="center">Sviluppo sistema IoT per il rilevamento della presenza umana dopo un terremoto</h3>
  

## 📝 Table of Contents

-  [About](#about)

-  [Getting Started](#getting_started)

-  [Deployment](#deployment)
  

## 🧐 About <a name = "about"></a>

  
Il progetto di ricerca industriale SAFE ha come obiettivo la realizzazione di sistemi di arredo innovativi capaci di trasformarsi in sistemi intelligenti di protezione passiva delle persone in caso di crollo dell'edificio causato da un terremoto.

Questi sistemi di arredo smart saranno dotati di sensoristica "salva-vita" capaci di rilevare e localizzare la presenza di vita dopo un crollo, di monitorare le condizioni ambientali sotto le macerie e di elaborare e trasmettere informazioni utili a chi deve portare soccorso.

Il ciclo di vita dei sensori si divide in tre scenari operativi:

 -  **Pre Evento:** monitoraggio per il pre-allertamento
 - **Evento:** invio dei dati per il rilevamento dei danni e attivazione di logiche di intervento in seguito al riconoscimento dell'evento.
 - **Post evento:** invio dei dati per la localizzazione delle vittime e monitoraggio ambientale al fine di guidare gli operatori nel triage di soccorso.

Lo scenario operativo "post evento" è caratterizzato da un'alta probabilità di interruzione dei servizi di telefonia e della rete elettrica, di crollo di parti dell'immobile e/o di caduta di parte degli arredi. Tale scenario si divide in tre attività:

 - **Campionatura:** mediante l'utilizzo di un drone dotato di tecnologia che supporta il protocollo LoRaWAN viene campionata l'area coperta dalle macerie. Durante la fase di volo vengono memorizzati i dati ricevuti dai sensori e la potenza del segnale.
 - **Analisi dati:** sfruttando opportuni algoritmi di localizzazione vengono analizzati i dati memorizzati dal drone, così da determinare dei centroidi in cui si suppone si trovi il disperso. 
 
 - **Guidare i soccoritori:** i soccoritori, dotati di opportuni tablet, visualizzerano una mappa con una heatmap, un'area e un marker per ogni sensore rilevato. Tali informazioni saranno visualizzate sulla base dei dati risultanti dall'attività di analisi, così da potersi orientare per individuare i dispersi.

Questo progetto, all'interno di SAFE, ha l'obbiettivo di contribuire allo sviluppo di un sistema IOT capace di rilevare le vittime bloccate negli edifici e garantire un pronto soccorso comunicando le informazioni al drone utilizzato per la campionatura.
   
Il sistema proposto si compone di due moduli principali: end-node, gateway.
L'**end-node** è un dispositivo wireless installato all'interno degli arredi smart che consentirà alle squadre di emergenza di rilevare la presenza e il respiro di persone all'interno del mobilio o nelle immediate vicinanze. Il **gateway** è un dispositivo wireless a bordo di un drone che sorvolerà le macerie consentendo la raccolta di informazioni provenienti dagli end-node.


## 🏁 Getting Started <a name = "getting_started"></a>

Queste istruzioni ti forniranno una copia del progetto e ti illustreranno come replicare il progetto per scopi di sviluppo e test. 

### Prerequisites (hardware)
 - [STM32 B-L072-LRWAN1](https://www.st.com/en/evaluation-tools/b-l072z-lrwan1.html)
 - [XeThru X4M200](https://novelda.com)
 - [PicoCell SX1308](https://www.semtech.com/products/wireless-rf/lora-core/sx1308p868gw)
 - [Raspberry Pi 3B+](https://www.raspberrypi.org)

### Prerequisites (software)
 - [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
 - [Rust](https://www.rust-lang.org/it) (installato su Raspberry Pi)
 - [Lora packet forwarder](https://github.com/Lora-net/picoGW_packet_forwarder) (installato su Raspberry Pi)
 - [ChirpStack](https://www.chirpstack.io) (installato su Raspberry Pi)

  
##  ⛏️ Architecture <a name = "architecture"></a>

### End Node
Il modulo **End-Node** è l'unione della board STM32 e il sensore Xethru collegati attraverso due coppie di cavi: alimentazione e trasferimento dati.
 ![Import](/screenshots/end-node.png)
| Pin Xethru | Colore | Pin STM32 |  
|--|--|--| 
| 1 | Rosso | 3v3 |
| 2 | Verde | GND |
| 3 | Fucsia | PA9 |
| 4 | Blu | PA10 |

### Gateway
Il modulo **Gateway** è costituito dal Raspberry Pi e dal Picocell SX1308 connessi mediante la porta USB.
 ![Import](/screenshots/gateway.png)
 
## 🚀 Deployment <a name = "deployment"></a>

### End node
Eseguire il clone del repository e collegare i .
Effettuare l'import del progetto in *STM32CubeIDE*: import > General > Existing Project into Workspace.

 ![Import](/screenshots/Import.png)
Selezionare il path corretto (..*/SafeProject/STM32CubeExpansion_LRWAN_V1.3.1_B-L072Z-LRWAN1/Projects/B-L072Z-LRWAN1/Applications/LoRa/End_Node/SW4STM32/mlm32l07x01*).

![Path](/screenshots/Path.png)

Completato l'import, è possibile eseguire l'upload del firmware all'interno dell'end node.
Attualmente il firmware presente è stato configurato in modalità ***ABP***. 
### Gateway

Ri-attivare il dispositivo resettando i counters di uplink e downlink:

 ![Riattivazione dispositivo](/screenshots/ReactivateDevice.png)

*(In modalità OTAA non è richiesta la riattivazione del dispositivo)*

Copiare ed incollare il payload decoder all'interno del Device Profile dell'end node nell'interfaccia ChirpStack:

	function Decode(fPort , bytes) { 
	    var profile = bytes[0]; 
	    var stateCode = bytes[1]; 
	     
	    if((profile == "0" && stateCode == "0") || (profile == "1" && stateCode < "3") )
	        return { 
	            "sensor":{ 
	                "Profile":profile, 
	                "StateCode":stateCode, 
	                "Distance": tofloat32([bytes[2],bytes[3],bytes[4],bytes[5]]), 
	                "Rpm": bytes[9]<<24 | bytes[8]<<16 | bytes[7]<<8 | bytes[6], 
	                "Movement": tofloat32([bytes[10],bytes[11],bytes[12],bytes[13]]), 
	                "SignalQuality": bytes[17]<<24 | bytes[16]<<16 | bytes[15]<<8 | bytes[14] 
	            } 
	        }; 
	    
	    return { 
	        "sensor":{ 
	            "Profile":profile, 
	            "StateCode":stateCode 
	        } 
	    }; 
	}
	 
	function tofloat32(bytes){ 
	    console.log(bytes); 
	    var bits = bytes[3]<<24 | bytes[2]<<16 | bytes[1]<<8 | bytes[0]; 
	    var sign = (bits>>>31 === 0) ? 1.0 : -1.0; 
	    var e = bits>>>23 & 0xff; 
	    var m = (e === 0) ? (bits & 0x7fffff)<<1 : (bits & 0x7fffff) | 0x800000; 
	    var f = sign * m *Math.pow(2 , e - 150); 
	    return f; 
	}

Una volta arrivati i pacchetti al Gateway saranno visibili all'interno di ChirpStack e inviati al broker MQTT installato insieme a ChirpStack nel topic:

> application/1/device/+/event/up

Per consentire la giusta formattazione dei messaggi MQTT eseguire il codice Rust presente nella cartella "mqtt", il quale publicherà il messaggio all'interno del topic:
 > /safe.it/jz/device/snapshot/0000/edv/ID_SENSORE
