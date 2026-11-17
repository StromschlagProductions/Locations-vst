// LocationsDSP.h - Klang-Engine fuer "Locations" (Stromschlag Audio)
// Reines C++ ohne JUCE, damit die Engine unabhaengig vom Plugin getestet werden kann.
//
// Eine einzige Signalkette, deren Werte gleitend von Ort A nach Ort B wandern:
//   Eingang -> Laufzeit (Weg) -> Tiefpass -> Pegel -> [Quelle, gepannt] + [Raum A] + [Raum B] -> Tuer -> Ausgang
// Ort A steht in der eingestellten Start-Entfernung, Ort B ist immer "nah".
// Weil es nur eine Kette gibt, entsteht in der Mitte der Bewegung kein Kammfilter (hohler Klang).
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace loc
{
constexpr float kPi = 3.14159265358979f;

inline float dbToLin (float db) { return db <= -79.0f ? 0.0f : std::pow (10.0f, db * 0.05f); }
inline float lerp (float a, float b, float t) { return a + (b - a) * t; }
inline float logLerp (float a, float b, float t) { return a * std::pow (b / a, t); }
inline float smooth01 (float x) { x = std::clamp (x, 0.0f, 1.0f); return x * x * (3.0f - 2.0f * x); }

//==============================================================================
struct PlaceDef
{
    const char* name;
    float fcNear, fcFar;       // Tiefpass-Grenzfrequenz nah / weit (Hz)
    float gainNear, gainFar;   // Pegel nah / weit (dB)
    float rt60;                // Nachhallzeit Raum der Quelle (s), 0 = kein Hall
    float dampHz;              // Hoehendaempfung im Hall (Hz)
    float size;                // Raumgroesse (Faktor auf die Laufzeiten)
    float wetNear, wetFar;     // Hallanteil nah / weit (dB)
    float delayMs;             // Laufzeit des Weges bei voller Entfernung (ms)
    float echoMs;              // Echo-Abstand (0 = kein Echo)
    float echoNear, echoFar;   // Echo-Pegel nah / weit (dB, <= -80 = aus)
    float echoFb;              // Echo-Rueckkopplung (Anzahl der Wiederholungen)
};

// Reihenfolge = Reihenfolge in der Auswahlliste (PluginProcessor::placeNames)
constexpr int kNumPlaces = 17;
inline const PlaceDef& place (int i)
{
    static const PlaceDef defs[kNumPlaces] = {
        //  Name                fcNah  fcWeit  gNah   gWeit   rt60  damp  size  wetN wetW  dly   echo  eN    eW    fb
        { "Gleicher Raum",      20000,  8000,   0.0f,  -6.0f, 0.00f, 6000, 1.0f, -80, -80,  0.0f,   0, -80,  -80, 0.0f },
        { "Nebenraum",           6500,  2500,  -2.5f,  -9.0f, 0.45f, 3500, 0.8f, -12,  -7,  6.0f,   0, -80,  -80, 0.0f },
        { "Flur",                7000,  2800,  -3.0f, -10.0f, 0.90f, 4500, 1.1f, -10,  -5,  8.0f,   0, -80,  -80, 0.0f },
        { "Treppenhaus",         5000,  2400,  -4.0f, -10.0f, 0.95f, 4500, 1.3f,  -9,  -6,  9.0f,   0, -80,  -80, 0.0f },
        { "Obere Etage",         2000,   900, -10.0f, -17.0f, 0.55f, 2500, 0.9f, -10,  -8, 14.0f,   0, -80,  -80, 0.0f },
        { "Dachboden",           2600,  1100,  -9.0f, -15.0f, 0.35f, 3000, 0.7f, -11,  -8, 14.0f,   0, -80,  -80, 0.0f },
        { "Keller",              1800,   800, -10.0f, -18.0f, 1.30f, 2000, 1.5f,  -7,  -3, 16.0f,   0, -80,  -80, 0.0f },
        { "Badezimmer",          5500,  2500,  -3.0f,  -9.0f, 0.70f, 7000, 0.5f,  -8,  -4,  5.0f,   0, -80,  -80, 0.0f },
        { "Grosse Halle",        9000,  3000,  -2.0f, -12.0f, 2.20f, 4000, 2.0f, -12,  -2, 10.0f,   0, -80,  -80, 0.0f },
        { "Tunnel / Hoehle",     9000,  3500,  -1.0f, -12.0f, 2.80f, 3500, 2.2f,  -9,   0, 12.0f,   0, -80,  -80, 0.0f },
        { "Kirche",              8000,  3000,  -2.0f, -12.0f, 3.20f, 3000, 2.3f, -10,  -1, 12.0f,   0, -80,  -80, 0.0f },
        { "Museum",             10000,  3500,  -2.0f, -12.0f, 1.80f, 5500, 1.8f, -12,  -3, 10.0f,   0, -80,  -80, 0.0f },
        { "Tiefgarage",          8000,  3000,  -2.0f, -12.0f, 2.00f, 4500, 1.6f,  -9,  -2, 10.0f,  35, -26,  -18, 0.25f },
        { "Draussen: Feld",     16000,  2200,   0.0f, -24.0f, 0.00f, 6000, 1.0f, -80, -80, 30.0f,   0, -80,  -80, 0.0f },
        { "Draussen: Wald",     12000,  1800,  -1.0f, -22.0f, 0.70f, 2800, 1.2f, -46, -32, 30.0f, 140, -32,  -13, 0.30f },
        { "Draussen: Strasse",  14000,  2500,   0.0f, -22.0f, 0.50f, 3500, 1.6f, -40, -28, 30.0f,  95, -20,  -10, 0.35f },
        { "Draussen: Schlucht", 14000,  2200,   0.0f, -22.0f, 1.40f, 4000, 1.5f, -44, -32, 30.0f, 420, -16,   -6, 0.45f },
    };
    return defs[std::clamp (i, 0, kNumPlaces - 1)];
}

constexpr int kFirstOutdoor = 13;   // ab diesem Index sind die Orte "Draussen"

//==============================================================================
// Was zwischen Quelle und Hoerer ist (Regler "zu - offen" blendet zwischen diesem Klang und offen)
struct BarrierDef
{
    const char* name;
    float fcClosed;   // Tiefpass bei geschlossen (Hz)
    float gainDb;     // Pegel bei geschlossen
    float hpHz;       // Bassabsenkung bei geschlossen
    float bodyHz;     // Eigenresonanz (Tuerblatt, Glas, Kabine)
    float bodyQ, bodyAmt;
};
constexpr int kNumBarriers = 7;
inline const BarrierDef& barrier (int i)
{
    static const BarrierDef defs[kNumBarriers] = {
        { "Tuer",         650.0f, -17.0f, 110.0f,  210.0f, 2.3f, 0.35f },
        { "Fenster",     1100.0f, -20.0f, 180.0f,  420.0f, 2.0f, 0.25f },
        { "Vorhang",     3500.0f,  -5.0f,  20.0f,  300.0f, 1.0f, 0.00f },
        { "Autoscheibe",  800.0f, -14.0f, 120.0f,  150.0f, 1.6f, 0.50f },
        { "Schrank",      900.0f, -12.0f,  90.0f,  320.0f, 2.5f, 0.45f },
        { "Kofferraum",   500.0f, -18.0f,  70.0f,  130.0f, 1.8f, 0.55f },
        { "Unter Wasser", 320.0f, -10.0f,  40.0f,  520.0f, 1.2f, 0.30f },
    };
    return defs[std::clamp (i, 0, kNumBarriers - 1)];
}

// Wetter draussen: zusaetzlicher Hoehenverlust, Pegelverlust und Boeen (Pegel schwankt langsam)
struct WeatherDef { const char* name; float lpOct, gainDb, gustDb, gustHz; };
constexpr int kNumWeathers = 3;
inline const WeatherDef& weather (int i)
{
    static const WeatherDef defs[kNumWeathers] = {
        { "Ruhig",         0.0f,  0.0f,  0.0f, 0.5f },
        { "Wind / Regen",  1.3f, -4.0f,  5.0f, 0.5f },
        { "Sturm",         2.6f, -9.0f, 12.0f, 1.0f },
    };
    return defs[std::clamp (i, 0, kNumWeathers - 1)];
}

//==============================================================================
// Zustandsvariablen-Filter (TPT), stabil bei bewegter Grenzfrequenz
struct SVF
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1.4142f;
    void reset() { ic1 = ic2 = 0; }
    void set (float fc, float sr, float q)
    {
        const float g = std::tan (kPi * std::min (fc, 0.49f * sr) / sr);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    inline void tick (float v0, float& lp, float& bp)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1;
    }
    inline float lowpass (float x) { float l, b; tick (x, l, b); return l; }
};

struct OnePole
{
    float z = 0, c = 1;
    void reset() { z = 0; }
    void set (float fc, float sr) { c = 1.0f - std::exp (-2.0f * kPi * fc / sr); }
    inline float lowpass (float x) { z += c * (x - z); return z; }
    inline float highpass (float x) { z += c * (x - z); return x - z; }
};

struct Delay
{
    std::vector<float> buf; int w = 0, len = 1;
    void prepare (int maxLen) { buf.assign ((size_t) std::max (4, maxLen + 2), 0.0f); w = 0; len = 1; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void setLength (int n) { len = std::clamp (n, 1, (int) buf.size() - 1); }
    inline float read() const { int r = w - len; if (r < 0) r += (int) buf.size(); return buf[(size_t) r]; }
    // gebrochene Verzoegerung in Samples (1 = juengstes Sample), fuer gleitende Laufzeit
    inline float readFrac (float d) const
    {
        const int size = (int) buf.size();
        d = std::clamp (d, 1.0f, (float) (size - 2));
        float r = (float) w - d;
        if (r < 0.0f) r += (float) size;
        int i0 = (int) r;
        const float f = r - (float) i0;
        if (i0 >= size) i0 -= size;
        int i1 = i0 + 1; if (i1 >= size) i1 = 0;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * f;
    }
    inline void write (float x) { buf[(size_t) w] = x; if (++w >= (int) buf.size()) w = 0; }
};

//==============================================================================
// Kleiner Raumhall: 8 gekoppelte Verzoegerungen (Householder), Daempfung im Kreis.
class Reverb
{
public:
    static constexpr int N = 8;
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& d : line) d.prepare ((int) (0.045f * 2.3f * sr) + 8);
        for (auto& d : ap)   d.prepare ((int) (0.012f * sr) + 8);
        set (0.5f, 4000.0f, 1.0f);
        reset();
    }
    void reset()
    {
        for (auto& d : line) d.reset();
        for (auto& d : ap) d.reset();
        for (auto& l : damp) l.reset();
    }
    void set (float rt60, float dampHz, float size)
    {
        static const float ms[N] = { 11.3f, 14.9f, 18.7f, 22.1f, 26.3f, 30.7f, 34.9f, 39.1f };
        float g2sum = 0;
        for (int i = 0; i < N; ++i)
        {
            const float sec = ms[i] * 0.001f * std::clamp (size, 0.4f, 2.3f);
            line[i].setLength ((int) (sec * sr));
            g[i] = std::pow (10.0f, -3.0f * sec / std::max (0.05f, rt60));
            g2sum += g[i] * g[i];
            damp[i].set (dampHz, sr);
        }
        ap[0].setLength ((int) (0.0047f * sr));
        ap[1].setLength ((int) (0.0079f * sr));
        norm = 0.43f * std::sqrt (std::max (1.0e-4f, 1.0f - g2sum / N));
    }
    inline void process (float in, float& outL, float& outR)
    {
        for (auto& a : ap)                       // Eingangs-Diffusion
        {
            const float d = a.read();
            const float v = in + 0.6f * d;
            a.write (v);
            in = d - 0.6f * v;
        }
        float o[N], sum = 0;
        for (int i = 0; i < N; ++i) { o[i] = line[i].read(); sum += o[i]; }
        sum *= 2.0f / N;
        static const float sIn[N] = { 1, -1, 1, 1, -1, 1, -1, -1 };
        float l = 0, r = 0;
        for (int i = 0; i < N; ++i)
        {
            const float fb = damp[i].lowpass ((o[i] - sum) * g[i]);
            line[i].write (in * sIn[i] + fb);
            if (i & 1) r += o[i] * sIn[i]; else l += o[i] * sIn[i];   // getrennte Abgriffe = links/rechts unkorreliert
        }
        outL = l * norm; outR = r * norm;
    }
private:
    float sr = 48000.0f, g[N] = {}, norm = 1.0f;
    Delay line[N], ap[2];
    OnePole damp[N];
};

//==============================================================================
// Raum eines Ortes (Hall + Echo). Bei Ortswechsel kurz aus- und wieder eingeblendet (kein Knacken).
class Room
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        rev.prepare (sr);
        echo.prepare ((int) (2.0f * sr));   // bis 2 s (Schlucht x4 = 1,7 s)
        echoLp.set (2500.0f, sr);
        echoGlide = 1.0f / (0.25f * sr);   // ca. 250 ms Gleitzeit
        current = -1;
        reset();
    }
    void reset() { rev.reset(); echo.reset(); echoLp.reset(); }
    void want (int p) { wanted = p; if (current < 0) apply(); }
    // Echo-Zeit-Faktor: wirkt nur bei Orten draussen (Wald, Strasse, Schlucht)
    void setEchoScale (float s) { scale = s; }

    inline void process (float in, float wet, float echoGain, float& l, float& r)
    {
        if (wanted != current)
        {
            fade -= 1.0f / (0.012f * sr);
            if (fade <= 0.0f) { fade = 0.0f; apply(); }
        }
        else if (fade < 1.0f) fade = std::min (1.0f, fade + 1.0f / (0.012f * sr));

        l = r = 0.0f;
        if (hasRev)
        {
            float a, b; rev.process (in, a, b);
            l += a * wet; r += b * wet;
        }
        if (hasEcho)
        {
            const float target = echoBase * (outdoor ? scale : 1.0f);
            echoLen += (target - echoLen) * echoGlide;       // gleitend: kein Knacken beim Verstellen
            const float e = echo.readFrac (echoLen);
            echo.write (in + echoLp.lowpass (e) * fb);
            l += e * echoGain; r += e * echoGain;
        }
        l *= fade; r *= fade;
    }
private:
    void apply()
    {
        current = wanted;
        const auto& p = place (current);
        reset();
        hasRev = p.rt60 > 0.0f;
        hasEcho = p.echoMs > 0.0f;
        rev.set (std::max (0.1f, p.rt60), p.dampHz, p.size);
        echoBase = std::max (1.0f, p.echoMs * 0.001f * sr);
        outdoor = current >= kFirstOutdoor;
        echoLen = echoBase * (outdoor ? scale : 1.0f);
        fb = p.echoFb;
    }
    float sr = 48000.0f, fade = 1.0f, fb = 0.0f;
    float scale = 1.0f, echoBase = 1.0f, echoLen = 1.0f, echoGlide = 0.0001f;
    int current = -1, wanted = 0;
    bool hasRev = false, hasEcho = false, outdoor = false;
    Reverb rev; Delay echo; OnePole echoLp;
};

//==============================================================================
struct Params
{
    int   placeA = 1, placeB = 0;
    float blend = 0.0f;        // Position: 0 = bei Ort A, 1 = bei Ort B
    float distance = 0.3f;     // Start-Entfernung von Ort A: 0 = nah, 1 = weit (Ort B ist immer nah)
    float door = 1.0f;         // 0 = zu, 1 = offen
    float roomDb = 0.0f;       // Raumanteil von Ort A (Trim, dB)
    float pan = 0.0f;          // -1 = links, 0 = Mitte, +1 = rechts (nur die Quelle, nicht der Raum)
    int   barrier = 0;         // Tuer / Fenster / Vorhang / Autoscheibe
    int   weather = 0;         // Ruhig / Wind-Regen / Sturm (wirkt nur draussen bei Ort A)
    float echoTime = 1.0f;     // Faktor auf die Echo-Zeit von Ort A, wenn draussen (0,25 bis 4)
};

class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        roomA.prepare (sr); roomB.prepare (sr);
        for (auto& d : path) d.prepare ((int) (0.04f * sr) + 4);
        smCoef = 1.0f - std::exp (-1.0f / (0.03f * sr / kBlock));   // 30 ms Glaettung
        first = true; firstBar = true; counter = 0;
        reset();
    }
    void reset()
    {
        roomA.reset(); roomB.reset();
        for (auto& d : path) d.reset();
        for (auto& f : lp) f.reset();
        for (auto& f : doorLp) f.reset();
        for (auto& f : doorHp) f.reset();
        for (auto& f : doorBp) f.reset();
    }
    // left/right duerfen identisch sein (Mono). Verarbeitung in-place.
    void process (float* left, float* right, int numSamples, const Params& p)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (counter == 0) control (p);
            if (++counter >= kBlock) counter = 0;

            curDly += dlyStep;
            path[0].write (left[i]); path[1].write (right[i]);
            const float xl = path[0].readFrac (curDly), xr = path[1].readFrac (curDly);
            float l = lp[0].lowpass (xl), r = lp[1].lowpass (xr);
            l += (xl - l) * open; r += (xr - r) * open;

            const float mono = 0.5f * (l + r);
            float al, ar, bl, br;
            roomA.process (mono, wetA, echoA, al, ar);
            roomB.process (mono, wetB, echoB, bl, br);
            l = (l * panL + al + bl) * gain;                 // Quelle gepannt, Raum bleibt stehen
            r = (r * panR + ar + br) * gain;

            if (doorOpen < 1.0f)
            {
                float x[2] = { l, r };
                for (int c = 0; c < 2; ++c)
                {
                    float y = doorHp[c].highpass (x[c]);
                    float lo, bp; doorBp[c].tick (y, lo, bp);
                    y = doorLp[c * 2 + 1].lowpass (doorLp[c * 2].lowpass (y)) + bp * doorBody;
                    y *= doorGain;
                    x[c] = y + (x[c] - y) * doorOpen;
                }
                l = x[0]; r = x[1];
            }
            left[i] = l;
            if (right != left) right[i] = r;
        }
    }
private:
    static constexpr int kBlock = 16;
    void control (const Params& p)
    {
        const float tBlend = std::clamp (p.blend, 0.0f, 1.0f);
        const float tDist = std::clamp (p.distance, 0.0f, 1.0f);
        const float tDoor = std::clamp (p.door, 0.0f, 1.0f);
        const float tPan = std::clamp (p.pan, -1.0f, 1.0f);
        const float es = std::clamp (p.echoTime, 0.25f, 4.0f);
        roomA.setEchoScale (es);   // nur Ort A (Ort B ist meist der gleiche Raum); vor want(): neue Orte starten gleich richtig
        roomA.want (p.placeA); roomB.want (p.placeB);

        const auto& A = place (p.placeA);
        const auto& B = place (p.placeB);
        if (first) { sBlend = tBlend; sDist = tDist; sDoor = tDoor; sPan = tPan; }
        sBlend += smCoef * (tBlend - sBlend);
        sDist += smCoef * (tDist - sDist);
        sDoor += smCoef * (tDoor - sDoor);
        sPan += smCoef * (tPan - sPan);

        // Zielwerte: Ort A in Start-Entfernung, Ort B nah, dazwischen gleitend
        const float t = sBlend, dA = sDist;
        // Wetter: nur wenn Ort A draussen ist, staerker mit Entfernung, verschwindet zu Ort B hin
        const auto& W = weather (p.weather);
        const float wAmt = (p.placeA >= kFirstOutdoor ? 1.0f : 0.0f) * dA * (1.0f - t);
        gustState = gustState + gustCoef * (rand01() * 2.0f - 1.0f - gustState);
        gustState2 = gustState2 + gustCoef * (gustState - gustState2);
        gustCoef = 1.0f - std::exp (-2.0f * kPi * W.gustHz * kBlock / sr);
        const float gustNorm = 0.7f / (0.577f * std::sqrt (gustCoef * 0.5f));
        const float gust = std::clamp (gustState2 * gustNorm, -1.0f, 1.0f);       // langsam schwankend, ca. -1..1
        const float fcT = logLerp (logLerp (A.fcNear, A.fcFar, dA), B.fcNear, t)
                          * std::pow (0.5f, W.lpOct * wAmt * (1.0f + 0.3f * gust));
        const float gT = lerp (lerp (A.gainNear, A.gainFar, dA), B.gainNear, t)
                         + (W.gainDb + W.gustDb * 0.5f * (gust - 1.0f)) * wAmt;
        const float ca = std::cos (t * kPi * 0.5f), cb = std::sin (t * kPi * 0.5f);   // Raeume: gleiche Leistung
        const float wAT = A.rt60 > 0.0f ? dbToLin (lerp (A.wetNear, A.wetFar, dA) + p.roomDb) * ca : 0.0f;
        const float wBT = B.rt60 > 0.0f ? dbToLin (B.wetNear) * cb : 0.0f;
        const float eAT = A.echoMs > 0.0f ? dbToLin (lerp (A.echoNear, A.echoFar, dA) + p.roomDb) * ca : 0.0f;
        const float eBT = B.echoMs > 0.0f ? dbToLin (B.echoNear) * cb : 0.0f;
        const float dlyT = 1.0f + lerp (A.delayMs * dA, 0.0f, t) * 0.001f * sr;

        if (first)
        {
            sLogFc = std::log (fcT); sGainDb = gT; wetA = wAT; wetB = wBT; echoA = eAT; echoB = eBT;
            curDly = sDly = dlyT; first = false;
        }
        sLogFc += smCoef * (std::log (fcT) - sLogFc);
        sGainDb += smCoef * (gT - sGainDb);
        wetA += smCoef * (wAT - wetA);  wetB += smCoef * (wBT - wetB);
        echoA += smCoef * (eAT - echoA); echoB += smCoef * (eBT - echoB);
        sDly += smCoef * (dlyT - sDly);
        dlyStep = (sDly - curDly) / (float) kBlock;

        const float fc = std::exp (sLogFc);
        for (auto& f : lp) f.set (fc, sr, 0.7071f);
        open = smooth01 ((fc - 12000.0f) / 8000.0f);      // nahe 20 kHz: Filter ganz umgehen
        gain = dbToLin (sGainDb);

        const float theta = (sPan + 1.0f) * kPi * 0.25f;   // Constant-Power, in der Mitte genau 0 dB
        panL = 1.41421356f * std::cos (theta);
        panR = 1.41421356f * std::sin (theta);

        // Verdeckung (Tuer, Fenster, Vorhang, Autoscheibe): zu = Klang des Hindernisses, offen = unveraendert
        const auto& D = barrier (p.barrier);
        if (firstBar) { bFc = std::log (D.fcClosed); bGain = D.gainDb; bHp = D.hpHz; bBodyHz = std::log (D.bodyHz); bQ = D.bodyQ; bAmt = D.bodyAmt; firstBar = false; }
        bFc += smCoef * (std::log (D.fcClosed) - bFc);  bGain += smCoef * (D.gainDb - bGain);   // Typwechsel gleitet
        bHp += smCoef * (D.hpHz - bHp);                  bBodyHz += smCoef * (std::log (D.bodyHz) - bBodyHz);
        bQ += smCoef * (D.bodyQ - bQ);                    bAmt += smCoef * (D.bodyAmt - bAmt);
        const float fcClosed = std::exp (bFc);
        const float closed = 1.0f - sDoor;
        const float dfc = fcClosed * std::pow (20000.0f / fcClosed, std::pow (sDoor, 1.4f));
        for (int c = 0; c < 2; ++c)
        {
            doorLp[c * 2].set (dfc, sr, 0.5412f);
            doorLp[c * 2 + 1].set (dfc, sr, 1.3066f);
            doorHp[c].set (20.0f + (bHp - 20.0f) * closed, sr);
            doorBp[c].set (std::exp (bBodyHz), sr, bQ);
        }
        doorBody = bAmt * closed * closed;
        doorGain = dbToLin (bGain * std::pow (closed, 1.2f));
        doorOpen = smooth01 ((sDoor - 0.9f) / 0.1f);
    }

    float sr = 48000.0f, smCoef = 1.0f;
    float sBlend = 0, sDist = 0.3f, sDoor = 1.0f, sPan = 0.0f;
    float sLogFc = 9.9f, sGainDb = 0.0f, wetA = 0, wetB = 0, echoA = 0, echoB = 0;
    float sDly = 1.0f, curDly = 1.0f, dlyStep = 0.0f;
    float open = 1.0f, gain = 1.0f, panL = 1.0f, panR = 1.0f;
    float doorBody = 0, doorGain = 1, doorOpen = 1;
    float bFc = 6.5f, bGain = 0.0f, bHp = 20.0f, bBodyHz = 5.3f, bQ = 1.0f, bAmt = 0.0f; bool firstBar = true;
    float gustState = 0.0f, gustState2 = 0.0f, gustCoef = 0.01f;
    unsigned int rng = 0x9e3779b9u;
    float rand01() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float) (rng & 0xffffff) / 16777216.0f; }
    bool first = true; int counter = 0;
    Room roomA, roomB;
    Delay path[2];
    SVF lp[2], doorLp[4], doorBp[2]; OnePole doorHp[2];
};
} // namespace loc
