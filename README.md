# Locations (Stromschlag Audio)

VST3-Effekt für Hörspiel-Produktion: lässt eine Stimme oder ein Geräusch so klingen, als käme es
aus einem anderen Raum, einer anderen Etage, hinter einer Tür oder aus der Entfernung im Freien.
Das Plugin erzeugt selbst keinen Klang, es verändert nur das Eingangssignal.

## Regler (alle automatisierbar)

| Regler | Wirkung |
|---|---|
| Ort A / Ort B | Start-Ort und Ziel-Ort der Bewegung (Ziel meist „Gleicher Raum“) |
| Position (A nach B) | **der eine Regler für die Bewegung**: 0 % = bei Ort A in Start-Entfernung, 100 % = bei Ort B ganz nah. Sind A und B gleich (z. B. Wald), ist es einfach „weit ↔ nah“ |
| Start-Entfernung | wie weit weg die Quelle bei Position 0 % ist |
| Tür (zu-offen) | 0 % = geschlossene Tür, 100 % = offen (keine Wirkung) |
| Raumanteil | Hall des Raums, in dem die Quelle steht (Trim in dB) |
| Pegelwirkung | wie stark der Pegel mit Entfernung/Tür sinkt (0 % = nur Klangfarbe) |
| Ausgang | Ausgangspegel |
| Weg-Verzögerung | wenige Millisekunden Laufzeit (aus = kein Zeitversatz) |

Der Hall des Hörraums (z. B. Wohnzimmer-IR) kommt wie gewohnt danach aus der DAW.

## Installation (Windows)

1. Auf GitHub: **Actions** → neuesten grünen Lauf öffnen → unter **Artifacts** `Locations-VST3-Windows` laden.
2. ZIP entpacken, den Ordner `Locations.vst3` nach `C:\Program Files\Common Files\VST3` kopieren.
3. DAW neu starten bzw. Plugins neu scannen.

## Aufbau

- `Source/LocationsDSP.h` – Klang-Engine (reines C++, Orte und Werte stehen oben in der Tabelle)
- `Source/PluginProcessor.*`, `Source/PluginEditor.*` – JUCE-Plugin und Oberfläche
- `.github/workflows/build.yml` – automatischer Windows-Bau mit Stabilitätstest (pluginval)

Gebaut mit [JUCE](https://juce.com) 8. Ideen zur Raum- und Entfernungssimulation angeregt durch die
MIT-lizenzierten Airwindows-Plugins (Distance, Cans) von Chris Johnson; kein Code daraus übernommen.
