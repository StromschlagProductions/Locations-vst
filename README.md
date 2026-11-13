# LOCATIONS – Audio Drama Off-Stage Simulator

*Stromschlag Audio*

VST3-Effekt für Hörspiel-Produktion. Er lässt eine Stimme oder ein Geräusch so klingen, als käme es aus
einem anderen Raum, einer anderen Etage, hinter einer geschlossenen Tür oder aus der Ferne im Freien –
und lässt es mit **einem** automatisierbaren Regler von dort heranrücken.

**⬇ Download (Windows, VST3):** https://github.com/StromschlagProductions/Locations-vst/releases/latest

## Oberfläche

| Dunkel (Standard) | Hell |
|---|---|
| ![Dunkle Oberfläche](docs/oberflaeche-dunkel.png) | ![Helle Oberfläche](docs/oberflaeche-hell.png) |

Der Umschalter oben rechts wechselt den Look, die Wahl wird mit dem Projekt gespeichert. Der **?**-Knopf
öffnet die Hilfe, über jedem Regler erscheint beim Darüberfahren ein kurzer Hinweis.

Die Oberfläche lässt sich am rechten unteren Fensterrand stufenlos von 50 % bis 300 % vergrößern oder
verkleinern. Das Seitenverhältnis bleibt dabei fest.

## So ist es gedacht

- **Ort A** ist der Ort, an dem die Quelle anfängt (z. B. Obere Etage, Wald).
- **Ort B** ist das Ziel, fast immer **Gleicher Raum** – der Raum, in dem die Zuhörer sitzen. Ort B ist immer „nah“.
- **Position** ist der eine Regler, den du automatisierst: 0 % = bei Ort A, 100 % = bei Ort B.
  Auf dem Weg dahin werden Klangfarbe, Lautstärke, Hall, Echo und Laufzeit gleitend von A nach B überführt.
- **Entfernung A** legt vorher fest, wie weit weg Ort A ist. So kannst du die Position immer von ganz links
  nach ganz rechts fahren und bestimmst trotzdem, ob der Förster tief im Wald oder hinter dem nächsten Baum anfängt.

Den Hall deines Hörraums (z. B. Wohnzimmer-Impulsantwort) legst du wie gewohnt danach in der DAW darüber.

## Orte

**Innen**

| Ort | Wann nehmen | Klang |
|---|---|---|
| Gleicher Raum | Die Figur ist im selben Raum wie die Zuhörer | Kein Effekt (nur bei Entfernung leicht dunkler und leiser) |
| Nebenraum | Durch eine Wand nebenan | Dumpfer, etwas Hall |
| Flur | Im Flur vor dem Raum | Heller als Nebenraum, mehr Hall |
| Treppenhaus | Auf der Treppe | Mittlerer, längerer Hall |
| Obere Etage | Oben, durch die Decke | Dumpf, leise, kurzer Hall |
| Dachboden | Unterm Dach | Dumpf, kurzer, trockener Raum |
| Keller | Unten im Keller | Dumpf, lang nachhallend |
| Badezimmer | Gekachelter kleiner Raum nebenan | Kurzer, heller, harter Hall |
| Große Halle | Weitläufiger Innenraum | Viel Hall, kaum Höhenverlust |
| Tunnel / Höhle | Tunnel, Stollen, Höhle | Sehr langer, harter Nachhall |

**Draußen** (kein Raumhall, sondern Höhenverlust mit der Entfernung und Echos)

| Ort | Wann nehmen | Klang |
|---|---|---|
| Draußen: Feld | Offene Fläche, Steppe, Outback | Kein Hall, nur Höhen- und Pegelverlust |
| Draußen: Wald | Im Wald, zwischen Bäumen | Kein Hall, weiches dumpfes Echo aus der Ferne |
| Draußen: Straße | Zwischen Häusern | Kurzes Echo von den Hauswänden, etwas Raum |
| Draußen: Schlucht / Felswand | Berge, Steinbruch, Felswand | Langes, wiederholtes Echo |

Die Laufzeit des Weges (ein paar Millisekunden innen, mehr draußen) und das Leiserwerden mit der Entfernung
passieren automatisch.

## Regler (alle automatisierbar)

| Regler | Wirkung |
|---|---|
| Ort A / Ort B | Start und Ziel der Bewegung |
| Position (A nach B) | **der Regler für die Bewegung**: 0 % = bei Ort A, 100 % = bei Ort B (nah) |
| Entfernung A (nah – weit) | wie weit weg Ort A ist. Wirkt nur auf Ort A |
| Tür (zu – offen) | zu = dumpf und leise, offen = keine Wirkung. Eigener Regler, weil eine Tür meist zu einem anderen Zeitpunkt aufgeht, als sich jemand bewegt |
| Panorama | platziert die Quelle links/rechts. Nur die Quelle wandert, der Raum (Hall, Echo) bleibt in der Mitte – anders als beim Pan am Kanal. Mitte = keine Wirkung |
| Raumanteil A | mehr oder weniger Hall bzw. Echo von Ort A (Feinregler, normalerweise 0 dB) |

## Anwendung

**Treppe herunter:** Ort A = Obere Etage, Ort B = Gleicher Raum. Position von 0 auf 100 % ziehen,
zu dem Zeitpunkt, an dem die Figur losgeht. Stimme, Schritte usw. am besten auf einen gemeinsamen Bus mit dem Plugin legen.

**Förster kommt aus dem Wald:** Ort A und B = Draußen: Wald, Entfernung A = weit. Position von 0 auf 100 % ziehen.
Steht er nur hinter dem nächsten Baum: Entfernung A auf etwa 40 %.

**Jemand geht weg:** wie oben, nur die Position von 100 auf 0 % ziehen.

**Vor der geschlossenen Tür:** Ort A = Flur, Ort B = Gleicher Raum, Position auf 0 %, Tür auf „zu“.
Geht die Tür auf: Tür von zu auf offen ziehen, danach ggf. die Position, wenn die Figur hereinkommt.

**Ruf aus der Schlucht:** Ort A und B = Draußen: Schlucht / Felswand, Entfernung A = weit, Position auf 0 % lassen.

## Installation (Windows)

1. Über den Download-Link oben `Locations-VST3-Windows.zip` laden.
2. ZIP entpacken und den **ganzen Ordner** `Locations.vst3` nach `C:\Program Files\Common Files\VST3` kopieren.
3. DAW neu starten bzw. Plugins neu scannen.

## Versionen

Die Versionsnummer steht in der Datei `VERSION`, im Plugin-Kopf und im Namen der Release. Sie wird bei jeder Änderung hochgezählt (1.0, 1.1, 1.2 …).

- **1.1** – Engine mit einer einzigen gleitenden Kette (kein hohler Klang mehr in der Mitte der Bewegung),
  Ort B immer nah, Entfernung und Raumanteil nur für Ort A, Wald ohne Hall, neue Orte Dachboden, Badezimmer,
  Tunnel / Höhle, Schlucht / Felswand, Regler „Ausgang“ entfernt.
  *Achtung:* Die Reihenfolge der Orte hat sich geändert – Projekte mit Version 1.0 bitte Ort A/B prüfen.
- **1.0** – Panorama nur für die Quelle, Hilfe, skalierbare Oberfläche, Hell/Dunkel.

## Aufbau

- `Source/LocationsDSP.h`: Klang-Engine (reines C++, Orte und Werte stehen oben in der Tabelle)
- `Source/PluginProcessor.*`, `Source/PluginEditor.*`: JUCE-Plugin und Oberfläche
- `.github/workflows/build.yml`: automatischer Windows-Bau mit Stabilitätstest (pluginval) und Release

Gebaut mit [JUCE](https://juce.com) 8. Ideen zur Raum- und Entfernungssimulation angeregt durch die
MIT-lizenzierten Airwindows-Plugins (Distance, Cans) von Chris Johnson; kein Code daraus übernommen.
