# May Pro Mix

**Assistente al mix per Fender Studio Pro** · *Mixing assistant for Fender Studio Pro*

Un progetto gratuito di Vince May, nato nei May Recording Studios.

May Pro Mix ascolta tutte le tracce del brano e ti dice, traccia per traccia, cosa sistemare: livello, equalizzazione, compressione, spazio tra gli strumenti e loudness finale. Ti suggerisce anche quali plug-in usare: prima quelli nativi di Fender Studio Pro, poi quelli che hai installato. Interfaccia in italiano e in inglese.

| Plug-in | Dove si inserisce |
| --- | --- |
| **May Pro Mix Track** | ultimo insert di ogni traccia |
| **May Pro Mix Master** | ultimo insert del canale Main |

Formati: VST3 (Windows e Mac), AU (Mac).

## Funzioni principali

- Loudness ITU-R BS.1770 (LUFS integrata e a breve termine), picchi, fattore di cresta, correlazione stereo e mono dei bassi
- Spettro a 8 bande confrontato con il profilo del ruolo (40 ruoli: band, 19 strumenti e sezioni d'orchestra, 5 tipi di pad, riconosciuti anche dal nome della traccia)
- 14 generi (rock, hard rock, pop, country, country rock, blues, folk, ballata, cantautorato, funk, jazz, elettronica, hip hop...) che cambiano livelli, timbro, compressione e consigli
- Bilanciamento tonale del mix con punti cliccabili sulle discrepanze e scala regolabile
- Consigli con colori di gravita', pulsante Applica per i livelli, Bilancia tutte nel Master e spunta Fatto con verifica e consiglio aggiuntivo
- Nessuna elaborazione audio: sulle schede di EQ e compressione un pulsante da trascinare inserisce nella catena insert il Pro EQ3 o il Compressor nativo di Studio Pro gia' impostato (preset generato al momento)
- Elenco delle tracce ordinate per problemi, riferimento personale, tutorial all'apertura

## Installazione rapida

1. Scarica il pacchetto per il tuo sistema dalla scheda **Actions** (ultima esecuzione con la spunta verde, sezione **Artifacts**).
2. Chiudi Fender Studio Pro.
3. Copia `May Pro Mix Track.vst3` e `May Pro Mix Master.vst3` in:
   - Windows: `C:\Program Files\Common Files\VST3`
   - Mac: `/Library/Audio/Plug-Ins/VST3` (i file `.component` in `/Library/Audio/Plug-Ins/Components`)
4. Solo Mac, nel Terminale:
   ```
   xattr -cr "/Library/Audio/Plug-Ins/VST3/May Pro Mix Track.vst3"
   xattr -cr "/Library/Audio/Plug-Ins/VST3/May Pro Mix Master.vst3"
   ```
5. Riapri Fender Studio Pro e, se serve, fai una nuova scansione dei plug-in.

Il manuale completo passo passo e' nella cartella `Docs`.

## Compilare da soli

Serve CMake 3.22 o superiore e un compilatore C++17; JUCE viene scaricato in automatico.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Quality check

Both plug-ins pass [pluginval](https://github.com/Tracktion/pluginval) at strictness level 10.

## English

May Pro Mix listens to every track of your song in Fender Studio Pro and tells you, track by track, what to fix and which plug-in to use. Insert **May Pro Mix Track** as the last insert on every track and **May Pro Mix Master** as the last plug-in on the Main channel, choose the genre, press Play and follow the advice. Switch the interface language with the IT / EN button. The full manual is in the `Docs` folder.

---

© 2026 Vince May · May Recording Studios · [vincemay.it](http://vincemay.it/)
