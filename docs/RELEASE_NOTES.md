# Locations 1.20 – Außenräume und technische Absicherung

- Markus Hildebrandts Außenraum-Vorschlag ist enthalten: deutlich weniger diffuser Hall, die diskreten Echos tragen Wald, Straße und Schlucht. Der Echo-Zeit-Regler im Kasten ORT A reicht von ×0,25 bis ×4; bei Innenorten ist er inaktiv. Schlucht bei ×4: ca. 1,68 s Echo-Abstand.
- Die gemeldete Nachklangzeit berücksichtigt nun die tatsächliche Echo-Zeit und Wiederholungen. So werden lange Schlucht-Echos beim Export nicht mehr durch die frühere pauschale 3-Sekunden-Angabe gefährdet.
- Unerwartet große Mono-Audioblöcke werden vollständig in vorreservierten Abschnitten verarbeitet, statt den Effekt unbemerkt zu überspringen.
- Ungültige Steuer-/Eingangswerte werden abgefangen; Reset setzt die Steuerzustände reproduzierbar zurück.
- Neue automatische DSP- und Prozessortests prüfen Orte, 22.050–192.000 Hz, Blockgrößen, Echo-Skalierung, State-Rundreise und Speicherallokationen. Der Windows-Release-Lauf prüft CTest und pluginval Level 8 vor Veröffentlichung.

Installation: ZIP entpacken, die DAW schließen und den ganzen Ordner `Locations.vst3` nach `C:\Program Files\Common Files\VST3` kopieren. Die bisherige Version bleibt im Release-Archiv erhalten.

Der Hallanteil ist bewusst nach Gehör abgestimmt, kein universelles physikalisches Hall-/Echo-Verhältnis von exakt 1:5. Hörkontrolle mit der eigenen Aufnahme bleibt Teil der Abmischung.
