# LOCATIONS – Audio Drama Off-Stage Simulator

*Stromschlag Audio*

VST3-Effekt für Hörspiel-Produktion. Er lässt eine Stimme oder ein Geräusch so klingen, als käme es aus
einem anderen Raum, einer anderen Etage, hinter einer geschlossenen Tür oder aus der Ferne im Freien.

**⬇ Download (Windows, VST3):** https://github.com/StromschlagProductions/Locations-vst/releases/latest

## Oberfläche

| Dunkel (Standard) | Hell |
|---|---|
| ![Dunkle Oberfläche](docs/oberflaeche-dunkel.png) | ![Helle Oberfläche](docs/oberflaeche-hell.png) |

Der Umschalter oben rechts wechselt den Look, die Wahl wird mit dem Projekt gespeichert.

Die Oberfläche lässt sich am rechten unteren Fensterrand stufenlos von 50 % bis 300 % vergrößern oder verkleinern. Das Seitenverhältnis bleibt dabei fest.

## Orte

Ort A ist der Startpunkt der Bewegung, Ort B das Ziel. In der Praxis ist Ort B meist **Gleicher Raum**,
also der Raum, in dem die Zuhörer sitzen.

**Innen**

| Ort | Wann nehmen | Klang |
|---|---|---|
| Gleicher Raum | Die Figur steht im selben Raum wie die Zuhörer | Kein Effekt, nur Klangfarbe bei Entfernung |
| Nebenraum | Durch eine Wand nebenan | Dumpfer, etwas Hall, kürzere Laufzeit |
| Flur | Im Flur vor dem Raum | Heller als Nebenraum, mehr Hall |
| Treppenhaus | Auf der Treppe, halb offen | Mittlerer Hall, deutlicher Weg-Klang |
| Obere Etage | Oben, durch die Decke | Dumpf und hallig, deutlich entfernt |
| Keller | Unten im Keller | Dumpf, lang nachhallend |
| Große Halle | Weitläufiger Innenraum | Viel Hall, kaum Höhenverlust |

**Draußen**

| Ort | Wann nehmen | Klang |
|---|---|---|
| Draußen: Feld | Offene Fläche, Steppe, Outback | Kaum Hall, Höhenverlust mit der Entfernung |
| Draußen: Wald | Im Wald, zwischen Bäumen | Leichter Hall, stark gedämpfte Höhen |
| Draußen: Straße | Zwischen Häusern | Ein deutlicher Echo-Reflex, Höhen weg |

## Regler

| Regler | Wirkung |
|---|---|
| Ort A / Ort B | Start- und Zielort der Bewegung |
| Position (A nach B) | **der eine Regler für die Bewegung**: 0 % = bei Ort A in Start-Entfernung, 100 % = bei Ort B ganz nah. Sind A und B gleich (z. B. Wald), ist es einfach „weit ↔ nah“ |
| Start-Entfernung | wie weit weg die Quelle bei Position 0 % ist |
| Tür (zu-offen) | 0 % = geschlossene Tür (dumpf, leise), 100 % = offen (keine Wirkung) |
| Panorama | platziert die Quelle links/rechts im Stereobild. Nur die Quelle wandert, der Raum (Hall, Echo) bleibt in der Mitte, anders als beim Pan am Kanal. Mitte = keine Wirkung. Automatisierbar |
| Raumanteil | Hall des Raums, in dem die Quelle steht (Trim in dB) |
| Ausgang | Ausgangspegel |
Die Lautstärke sinkt mit der Entfernung und bei geschlossener Tür automatisch. Der Laufweg (z. B. Treppenhaus) verzögert den Ton automatisch um wenige Millisekunden, bei Gleicher Raum gar nicht.

## Anwendung

**Treppe herunter:** Ort A = Obere Etage, Ort B = Gleicher Raum, Start-Entfernung nach Gefühl. Position in Studio One
von 0 auf 100 % ziehen, zu dem Zeitpunkt, an dem die Figur losgeht.

**Förster kommt aus dem Wald:** Ort A und B = Draußen: Wald, Start-Entfernung weit (z. B. 100 %), Position von 0 auf 100 %
ziehen. Die Stimme wird nach und nach größer.

**Vor der geschlossenen Tür:** Ort A = Flur, Ort B = Gleicher Raum, Tür auf 0 % stellen. Geht die Tür auf, Tür-Regler
von 0 auf 100 % ziehen, Position bleibt.

**Jemand ruft aus der Steppe:** Ort A = Draußen: Feld, Ort B = Draußen: Feld, Position von 100 % auf 0 %
ziehen, die Stimme kommt näher.

Die Tür bleibt ein eigener Regler, weil sie meist zu einem anderen Zeitpunkt aufgeht als sich die Figur bewegt.

Die Wohnzimmer-Impulsantwort legst du danach wie gewohnt in der DAW darüber. Das Plugin enthält keinen Hörraum-Hall.

## Installation (Windows)

1. Unter **Actions** den neuesten grünen Lauf öffnen und unter **Artifacts** `Locations-VST3-Windows` laden.
2. ZIP entpacken und den Ordner `Locations.vst3` nach `C:\Program Files\Common Files\VST3` kopieren.
3. DAW neu starten bzw. Plugins neu scannen.

## Aufbau

- `Source/LocationsDSP.h`: Klang-Engine (reines C++, Orte und Werte stehen oben in der Tabelle)
- `Source/PluginProcessor.*`, `Source/PluginEditor.*`: JUCE-Plugin und Oberfläche
- `.github/workflows/build.yml`: automatischer Windows-Bau mit Stabilitätstest (pluginval)

Gebaut mit [JUCE](https://juce.com) 8. Ideen zur Raum- und Entfernungssimulation angeregt durch die
MIT-lizenzierten Airwindows-Plugins (Distance, Cans) von Chris Johnson; kein Code daraus übernommen.
