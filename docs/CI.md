# CI-Nutzung (GitHub Actions)

Stand: 2026-11-16. Das GitHub-Konto hat nur noch wenige der enthaltenen Actions-Minuten. Windows-Läufe zählen doppelt.

| Vorher | Jetzt |
|---|---|
| Windows-Bau + pluginval bei **jedem Push** auf `main` (außer Doku) | Nur auf Anforderung (`workflow_dispatch`) |
| Release **automatisch** nach jedem erfolgreichen Lauf | Release nur, wenn beim Start **„publish“** angehakt wird (Standard: aus), nur auf `main` |
| Timeout 45 Minuten | 30 Minuten |
| Überholte Läufe liefen zu Ende | `concurrency` bricht überholte Läufe ab (nicht den Release-Lauf) |
| Artefakt ohne Frist | Artefakt 7 Tage |

Unverändert: Windows-Bau und pluginval Strictness 8.

## Ablauf

1. Lokal entwickeln und testen.
2. Zur Abnahme: GitHub > Actions > „Windows-VST3 bauen“ > „Run workflow“ (Branch `main`), zunächst ohne Haken. Das Artefakt lässt sich herunterladen.
3. Wenn alles passt: derselbe Lauf mit Haken **publish**. Das veröffentlicht das Release mit ZIP und setzt den Download-Link der README („latest“) auf die neue Version.

Es wird nie automatisch ein Release oder Merge ausgelöst.
