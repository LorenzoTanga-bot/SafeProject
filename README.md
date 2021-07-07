# I CUBE LRWAN 2.0.0

In questa wiki è stata descritta step by step la procedura che consente l'upload del sorgente I-CUBE-LRWAN 2.0.0 nella board STM32 B-L072-LRWAN1

## Prerequisiti (hardware)
- STM32 B-L072-LRWAN1

## Prerequisiti (software)
- I-CUBE-LRWAN 2.0.0 (https://www.st.com/en/embedded-software/i-cube-lrwan.html)

## Deployment

Eseguire il clone del branch **i-cube-lrwan-2.0.0**.
Effettuare l'import del progetto in *STM32CubeIDE*

 ![Import](/screenshots/Import.png)`

Selezionare il path corretto (..*/STM32CubeExpansion_LRWAN_V2.0.0_B_L072Z-LRWAN1\Projects\B-L072Z-LRWAN1\Applications\LoRaWAN\LoRaWAN_End_Node\STM32CubeIDE\cmwx1zzabz_0xx*).

 ![Path](/screenshots/Path.png)`

Non appena l'import è stato completato, è possibile eseguire l'upload del firmware all'interno dell'end node.
Attualmente il firmware presente è stato configurato in modalità ***ABP***.  

Qualora si desideri usare la modalità ***OTAA***  sarà sufficiente spostarsi nel file *lora_app.h* (raggiungibile da progetto al path *../STM32CubeExpansion_LRWAN_V2.0.0_B_L072Z-LRWAN1\Projects\B-L072Z-LRWAN1\Applications\LoRaWAN\LoRaWAN_End_Node\LoRaWAN\App* )  e modificare la riga:

```console
#define LORAWAN_DEFAULT_ACTIVATION_TYPE             ACTIVATION_TYPE_ABP
```

sostituendo la stringa ***ACTIVATION_TYPE_ABP*** con **ACTIVATION_TYPE_OTAA**

Se la modalità usata è ABP, ricordarsi di riattivare il dispositivo resettando i counters dopo averlo ricollegato:

 ![Riattivazione dispositivo](/screenshots/ReactivateDevice.png)`

In modalità OTAA non è richiesta la riattivazione del dispositivo.

A prescindere dalla modalità, copiare ed incollare il payload decoder all'interno del Device Profile dell'end node nell'interfaccia ChirpStack:

```console
function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  // (array) of bytes to an object of fields.
  var decoded = {};
  
  // temperature 
 
  nstate1 = bytes[0] + bytes[1] bytes[2] + bytes[3] * 256;
  decoded.nstate1 = sflt162f(nstate1) * 100; 
  
  nstate2 = bytes[4] + bytes[5] + bytes[6] + bytes[7] * 256;
  decoded.nstate2 = sflt162f(nstate2) * 100;  
  
  nstate3 = bytes[8] + bytes[9] + bytes[10] + bytes[11] * 256;
  decoded.nstate3 = sflt162f(nstate3) * 100;  

  rpm = bytes[12] + bytes[13]   + bytes[14] + bytes[15] * 256;
  decoded.nstate1 = sflt162f(rpm) * 100;  
  // humidity 
  distance = bytes[16] + bytes[17] + bytes[18] + bytes[19] * 256;
  decoded.distance = sflt162f(distance) * 100;
  
  return decoded;
}
 
function sflt162f(rawSflt16)
	{
	// rawSflt16 is the 2-byte number decoded from wherever;
	// it's in range 0..0xFFFF
	// bit 15 is the sign bit
	// bits 14..11 are the exponent
	// bits 10..0 are the the mantissa. Unlike IEEE format, 
	// 	the msb is transmitted; this means that numbers
	//	might not be normalized, but makes coding for
	//	underflow easier.
	// As with IEEE format, negative zero is possible, so
	// we special-case that in hopes that JavaScript will
	// also cooperate.
	//
	// The result is a number in the open interval (-1.0, 1.0);
	// 
	
	// throw away high bits for repeatability.
	rawSflt16 &= 0xFFFF;
 
	// special case minus zero:
	if (rawSflt16 == 0x8000)
		return -0.0;
 
	// extract the sign.
	var sSign = ((rawSflt16 & 0x8000) != 0) ? -1 : 1;
	
	// extract the exponent
	var exp1 = (rawSflt16 >> 11) & 0xF;
 
	// extract the "mantissa" (the fractional part)
	var mant1 = (rawSflt16 & 0x7FF) / 2048.0;
 
	// convert back to a floating point number. We hope 
	// that Math.pow(2, k) is handled efficiently by
	// the JS interpreter! If this is time critical code,
	// you can replace by a suitable shift and divide.
	var f_unscaled = sSign * mant1 * Math.pow(2, exp1 - 15);
 
	return f_unscaled;
	}
```


Attualmente il payload decoder installato nel Device Profile è molto semplice (configurazione di minima per far comprendere il funzionamento, viene inviato il valore relativo al livello della batteria dell'end node).

![Codec - Payload Decoder](/screenshots/PayloadDecoder.png)

Il payload decoder condiviso in questa wiki andrà testato con il pacchetto inviato nella funzione **Send** del firmware in cui dovranno essere presenti anche i dati inviati mediante UART dal modulo Xethru.
Sarà sufficiente il porting delle funzioni custom già implementate nella versione I-CUBE-LRWAN 1.3.1 in I-CUBE-LRWAN 2.0.0

I pacchetti in modalità ABP/OTAA sono visibili in ChirpStack ed inviati al broker MQTT:

![ABP](/screenshots/ABP.png)

Con la modalità OTAA si vedrà nell'interfaccia ChirpStack anche il pacchetto di JOIN:
![OTAA](/screenshots/OTAA.png)