# HBW-1W-T10

10-Kanal-1-Wire-Temperaturmodul für HomeMatic Wired

## Überblick

HomeMatic-Wired-Gerät (RS485) mit bis zu 10 DS18x20-Temperatursensoren an einem
gemeinsamen 1-Wire-Bus. Die Sensoren werden **automatisch erkannt**: Beim Start
scannt das Gerät den Bus, übernimmt die ROM-Adressen jedes gefundenen Fühlers und
legt sie in freie Kanäle. In der CCU wird nur noch eingestellt, wann gesendet wird.

Gerätetyp `0x81`, Aufbau auf zwei Platinen (Versorgung/Bus und Controller).

### Basiert auf:
- **HBWired** von Thorsten Pferdekaemper: https://github.com/ThorstenPferdekaemper/HBWired
- Sketch-Vorlage von **loetmeister.de** (Thorsten Pferdekaemper, Dirk Hoffmann)
- Identify-LED nach einer Idee von **jfische**: https://github.com/jfische/HBW-1W-T10

## Aufbau des Repositories

```
HBW-1W-T10/
├── HBW-1W-T10/                  Arduino-Sketch (Ordnername muss zur .ino passen)
│   ├── HBW-1W-T10.ino
│   └── HBW-1W-T10_config.h      Pinbelegung dieser Platine
├── hbw_1w_t10_v1.xml            CCU-Gerätedefinition (hs485types)
├── Platine1/                    KiCad-Projekt Versorgung + RS485 (+ Gerber)
├── Platine2/                    KiCad-Projekt Controller (+ Gerber)
└── README.md
```

## Hardware

### Unterstützte Sensoren

| Sensor | Family-Code | Auflösung | Genauigkeit |
|--------|-------------|-----------|-------------|
| **DS18B20** | 0x28 | 9–12 Bit (Werksvorgabe 12) | ±0,5 °C |
| **DS18S20** / DS1820 | 0x10 | 9 Bit + Extended Resolution | ±0,5 °C |
| **DS1822** | 0x22 | 9–12 Bit | ±2 °C |
| **DS1825** | 0x3B | 9–12 Bit | ±0,5 °C |

Bis zu 10 Fühler parallel an einem Bus (Daten, VCC, GND). DS18S20 wird intern
anders umgerechnet als die übrigen — er liefert 9 Bit in 0,5-°C-Schritten, die
über COUNT_REMAIN auf volle Auflösung hochgerechnet werden.

> **DS1825** setzt einen kleinen Patch in HBWired voraus, siehe [Installation](#2-ds1825-freischalten-optional).

### Bauteile

**Platine1** (Versorgung und Bus)
- **MAX487E** RS485-Transceiver
- **MC34063AD** Step-Down-Wandler (24 V Bus → 5 V), 270 µH, SB140
- Feinsicherung 375 mA, SM4007 Verpolschutz
- Schraubklemmen für 24 V und Bus

**Platine2** (Controller)
- **ATmega328P-A** mit 16-MHz-Resonator
- ISP-Programmierstecker (2×3)
- Status-LED, Identify-LED, Taster für Reset und Config
- Steckverbinder zu Platine1 (Nano-Pinout + RS485-Signale)

## Pinbelegung

| Pin | Netz im Schaltplan | Funktion |
|-----|--------------------|----------|
| D2 | `TXEN` | RS485 Transmit-Enable |
| D5 | `Button` | Taster für Config/Werksreset |
| D10 | `OneWire` | 1-Wire-Datenleitung (4,7 kΩ Pull-up nach VCC) |
| D12 / PB4 | `ID_LED` | Identify-LED |
| D13 | `LED` | Status-LED |
| D0 / D1 | `RXD` / `TXD` | UART0 zum MAX487E |

Die SoftwareSerial-Variante von HBWired kommt hier
nicht in Frage (die bräuchte D2 als TX) — das Modul läuft mit
`USE_HARDWARE_SERIAL` über UART0.

## Funktionsumfang

### Konfiguration je Kanal (in CCU/FHEM)

| Parameter | Bereich | Vorgabe | Bedeutung |
|-----------|---------|---------|-----------|
| **ONEWIRE_TYPE** | NOT_USED / DS18B20 / DS18S20 / DS1822 / DS1825 / REMOVE_SENSOR | NOT_USED | Anzeige des erkannten Sensortyps; `REMOVE_SENSOR` gibt den Kanal wieder frei |
| **SEND_DELTA_TEMP** | 0,1–25,0 °C | 0,5 °C | Sendet, sobald sich die Temperatur um diesen Betrag ändert |
| **OFFSET** | −1,27 bis +1,27 °C | 0,0 °C | Kalibrier-Offset (vorzeichenbehaftet, mit Bias 127 gespeichert) |
| **SEND_MIN_INTERVAL** | 5–3600 s | 10 s | Mindestabstand zwischen zwei Sendungen |
| **SEND_MAX_INTERVAL** | 5–3600 s | 150 s | Spätestens nach dieser Zeit wird gesendet |

### Konfiguration des Geräts

| Parameter | Vorgabe | Bedeutung |
|-----------|---------|-----------|
| **IDENTIFY_LED** | aus | LED an D12 blinkt im 600-ms-Takt, solange gesetzt — zum Auffinden des Moduls in der Verteilung |
| **OWN_ADDRESS** | 1 | Busadresse des Geräts |

### ONEWIRE_TYPE ist kein Einstellwert

Der Parameter liegt auf dem ersten Byte der 1-Wire-ROM-Adresse — dem Family-Code.
Er zeigt also an, was der Bus-Scan gefunden hat, und wird nicht von Hand gesetzt:
Ein hier eingetragener Typ ändert nur dieses eine Byte, die restlichen sieben
(Seriennummer und CRC) bleiben stehen, und der Kanal spricht danach eine Adresse
an, die es auf dem Bus nicht gibt.

Sinnvoll zu benutzen ist einzig **`REMOVE_SENSOR`**: Das schreibt 0xFF, macht die
gespeicherte Adresse ungültig und gibt den Kanal für einen neuen Fühler frei. Beim
Zurücklesen steht dort anschließend `NOT_USED` — es ist ein einmaliger Befehl,
kein Zustand.

### Ablauf

1. **Bus-Scan** bei jedem Start und nach jedem Konfigurationsschreiben der
   Zentrale: neue Sensoren wandern in freie Kanäle, ihre ROM-Adresse wird ins
   EEPROM übernommen.
2. **Messung**: Alle 1200 ms ist ein Kanal an der Reihe — erst Konvertierung
   anstoßen, im nächsten Durchgang das Ergebnis lesen. Es läuft immer nur eine
   Konvertierung gleichzeitig auf dem gemeinsamen Bus.
3. **Sendeentscheidung**: nie vor SEND_MIN_INTERVAL; gesendet wird bei einer
   Änderung ≥ SEND_DELTA_TEMP oder spätestens nach SEND_MAX_INTERVAL.

Die Kanalzuordnung ergibt sich aus der Reihenfolge, in der die Sensoren beim Scan
gefunden werden — und die hängt an den ROM-Adressen, nicht an der Position im
Strang. Wer eine bestimmte Zuordnung braucht, steckt die Fühler einzeln
nacheinander an oder räumt Kanäle gezielt über `REMOVE_SENSOR` frei.

## Installation

### 1. Arduino-IDE vorbereiten
```bash
git clone https://github.com/maxx3105/HBWired
# nach Arduino/libraries/HBWired kopieren
```

### 2. DS1825 freischalten (optional)

DS18B20, DS18S20 und DS1822 laufen mit HBWired unverändert. Für den **DS1825**
sind zwei kleine Ergänzungen in der Bibliothek nötig, da sie den Family-Code sonst
verwirft:

`libraries/src/HBWOneWireTempSensors.h`
```diff
     static const uint8_t DS18B20_ID = 0x28;
+    static const uint8_t DS1825_ID = 0x3B;
```

`libraries/src/HBWOneWireTempSensors.cpp`
```diff
 bool HBWOneWireTemp::deviceInvalidOrEmptyID(uint8_t deviceType) {
-  return !((deviceType == DS18B20_ID) || (deviceType == DS18S20_ID) || (deviceType == DS1822_ID));
+  return !((deviceType == DS18B20_ID) || (deviceType == DS18S20_ID) || (deviceType == DS1822_ID) ||
+           (deviceType == DS1825_ID));
 };
```

Die Umrechnung braucht keinen eigenen Zweig: Der DS1825 hält seine
Auflösungsbits an derselben Stelle wie der DS18B20 (`data[4] & 0x60`), womit der
bestehende `else`-Zweig greift.

### 3. Übersetzen und flashen

Der Sketch liegt im Unterordner `HBW-1W-T10/` (der Ordnername muss dem
`.ino`-Namen entsprechen).

- `HBW-1W-T10/HBW-1W-T10.ino` in der Arduino-IDE öffnen
- Board: **Arduino Nano** (ATmega328P)
- `HBW-1W-T10_config.h` bildet die Platine ab; `USE_HARDWARE_SERIAL` ist gesetzt.
  Zum Testen auf einem Nano mit Debug-Ausgabe auskommentieren.
- Flashen über ISP

Oder aus dem Projektverzeichnis heraus:

```
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 HBW-1W-T10
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 --upload --programmer usbasp HBW-1W-T10
```

Hinweis: Ist die EESAVE-Fuse nicht gesetzt, löscht jeder ISP-Flash das EEPROM —
und damit die Busadresse und alle gespeicherten Sensoradressen.

### 4. CCU/RaspberryMatic einrichten
1. https://github.com/maxx3105/JP-HB-Devices-addon
2. JP-HB-Devices-addon installieren
3. Gerät an den RS485-Bus anschließen
4. Anlernen und aus dem Posteingang übernehmen

## EEPROM-Belegung

`HBWDevice` liest die Konfigurationsstruktur ab EEPROM-Adresse `0x01`
(siehe `readConfig()` in HBWired.cpp), Struktur-Offset 6 landet also auf `0x07`.

```
0x00      : ungenutzt
0x01      : Logging-Zeit
0x02-0x05 : Zentralen-Adresse
0x06      : Bit 0 Direct-Link-Flag, Bit 1 Identify-LED (invertiert gespeichert)
0x07      : Kanal 1, Konfiguration (14 Byte)
  +0: send_delta_temp
  +1: offset (Bias 127: 0 = −1,27 °C, 127 = 0,00 °C, 254 = +1,27 °C)
  +2-3: send_min_interval (16 Bit, little endian)
  +4-5: send_max_interval (16 Bit, little endian)
  +6-13: 1-Wire-ROM-Adresse; Byte +6 ist der Family-Code = ONEWIRE_TYPE
0x15/0x23/... : Kanal 2..10, Konfiguration (Schrittweite 14), bis 0x92
0x3FC-0x3FF : OWN_ADDRESS (Busadresse, E2END−3 beim 1-kB-EEPROM des 328P)
```

Die Identify-LED wird **invertiert** abgelegt (`boolean_integer invert="true"` in
der XML): Nach einem Werksreset steht das EEPROM auf 0xFF, das Flag also auf 1 —
und die LED bleibt aus. Deshalb heißt das Feld im Sketch `n_identify_led`.

Ein `static_assert` in `HBW-1W-T10.ino` koppelt `ADDRESS_START_CONF_TEMP_CHAN` an
die tatsächliche Position von `TempOWCfg` in der Struktur. Käme ein Feld davor
hinzu, würde `sensorSearch()` sonst still in die Kanalkonfiguration schreiben —
der Build bricht stattdessen ab.

## Fehlersuche

### Sensoren werden nicht gefunden
- Verdrahtung prüfen (3-adrig, kein Parasite-Power-Betrieb vorgesehen)
- Nach dem Anstecken Reset drücken oder in der CCU die Konfiguration schreiben —
  beides löst einen Bus-Scan aus
- Fremde 1-Wire-Bausteine am Bus belegen kurzzeitig einen Kanal, bis ein echter
  Fühler den Platz übernimmt

### Temperatur zeigt −273,15 °C oder −273,14 °C
Das sind die beiden Sonderwerte der Firmware: **−273,15 °C** heißt „Kanal ohne
gültigen Sensor", **−273,14 °C** steht für einen Lese- oder CRC-Fehler, der drei
Versuche überdauert hat — typisch bei abgezogenem Fühler, zu langer Leitung oder
fehlendem Pull-up.

### Falsche Kanalzuordnung
Erwartungsgemäß: Die Reihenfolge kommt vom Bus-Scan, nicht von der Verdrahtung.
Über `REMOVE_SENSOR` einzelne Kanäle freiräumen und die Fühler nacheinander
anstecken.

### Gerät kommuniziert nicht
- RS485-Verdrahtung prüfen (A/B nicht vertauscht)
- Busadresse gesetzt? (`OWN_ADDRESS`, Seriennummer ist 7-Stellig und darf nur einmal am Bus vorhanden sein)
- Ist `USE_HARDWARE_SERIAL` aktiv? Ohne die Option belegt SoftwareSerial D2, das
  auf dieser Platine das Transmit-Enable ist.

### Modul in der Verteilung finden
`IDENTIFY_LED` in der Gerätekonfiguration setzen — die LED an D12 blinkt dann im
600-ms-Takt, bis der Haken wieder entfernt wird.

## Lizenz

Creative Commons BY-NC-SA 3.0 AT
http://creativecommons.org/licenses/by-nc-sa/3.0/at/

## Dank an

- Thorsten Pferdekaemper – HBWired-Framework
- Dirk Hoffmann – Beiträge zu HBWired
- loetmeister.de – Sketch-Vorlage HBW-1W-T10
- jfische – Idee der Identify-LED

## Änderungsverlauf

### v0.06
- 1-Wire-Busarbitrierung repariert: Die Kanäle hielten Zeiger auf lokale
  Variablen von `setup()`, deren Speicher freigegeben wird, sobald `loop()`
  läuft (auch upstream eingereicht:
  [HBWired#47](https://github.com/ThorstenPferdekaemper/HBWired/pull/47))
- `static_assert` koppelt die Kanaladresse an das Struktur-Layout

### v0.05
- DS1825 als vierter Sensortyp

### v0.04
- Identify-LED an D12, schaltbar über den Geräteparameter `IDENTIFY_LED`

### v0.03
- Prüfung auf unterstützte Sensortypen
- Konvertierungs- und Messablauf entzerrt, um falsche Messwerte zu vermeiden

### v0.02
- Start und Fehlerbehandlung bei abgezogenen Sensoren verbessert

### v0.01
- Erste Fassung
