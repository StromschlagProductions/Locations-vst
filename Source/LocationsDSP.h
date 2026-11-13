// LocationsDSP.h - Klang-Engine fuer "Locations" (Stromschlag Audio)
// Reines C++ ohne JUCE, damit die Engine unabhaengig vom Plugin getestet werden kann.
//
// Kette:  Eingang -> Ort A und Ort B (parallel) -> Blende A<->B -> Tuer -> Ausgang
// Ein Ort = Tiefpass (Entfernung/Wand) + Raum der Quelle (kleiner Hall) + Echo (nur Strasse)
//           + optionale Weg-Verzoegerung + Pegel.
// Die Engine erzeugt selbst keinen Klang; sie veraendert nur das Eingangssignal.
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
    float rt60;                // Nachhallzeit Raum der Quelle (s), 0 = kein Raum
    float dampHz;              // Hoehendaempfung im Hall (Hz)
    float size;                // Raumgroesse (Faktor auf die Laufzeiten)
    float wetNear, wetFar;     // Hallanteil nah / weit (dB, <= -80 = aus)
    float delayMs;             // Weg-Verzoegerung (nur wenn eingeschaltet)
    float echoMs;              // Einzel-Echo (0 = keins)
    float echoNear, echoFar;   // Echo-Pegel nah / weit (dB)
};

constexpr int kNumPlaces = 10;
inline const PlaceDef& place (int i)
{
    static const PlaceDef defs[kNumPlaces] = {
        //  Name               fcNah  fcWeit  gNah   gWeit  rt60  damp   size  wetNah wetWeit dly   echo  eNah  eWeit
        { "Gleicher Raum",     20000,  8000,   0.0f,  -6.0f, 0.0f, 6000, 1.0f, -80,   -80,    0.0f,  0,   -80,  -80 },
        { "Nebenraum",          6500,  2500,  -2.5f,  -9.0f, 0.45f, 3500, 0.8f, -12,   -7,     4.0f,  0,   -80,  -80 },
        { "Flur",               7000,  2800,  -3.0f, -10.0f, 0.90f, 4500, 1.1f, -10,   -5,     5.0f,  0,   -80,  -80 },
        { "Treppenhaus",        5000,  2400,  -4.0f, -10.0f, 0.95f, 4500, 1.3f,  -9,   -6,     6.0f,  0,   -80,  -80 },
        { "Obere Etage",        2000,   900, -10.0f, -17.0f, 0.55f, 2500, 0.9f, -10,   -8,    14.0f,  0,   -80,  -80 },
        { "Keller",             1800,   800, -10.0f, -18.0f, 1.30f, 2000, 1.5f,  -7,   -3,    16.0f,  0,   -80,  -80 },
        { "Grosse Halle",       9000,  3000,  -2.0f, -12.0f, 2.20f, 4000, 2.0f, -12,   -2,     8.0f,  0,   -80,  -80 },
        { "Draussen: Feld",    16000,  2200,   0.0f, -24.0f, 0.0f, 6000, 1.0f, -80,   -80,    0.0f,  0,   -80,  -80 },
        { "Draussen: Wald",    12000,  1800,  -1.0f, -22.0f, 0.70f, 2800, 1.8f, -22,   -9,     0.0f,  0,   -80,  -80 },
        { "Draussen: Strasse", 14000,  2500,   0.0f, -22.0f, 0.50f, 3500, 1.6f, -24,  -13,     0.0f, 95,   -20,  -10 },
    };
    return defs[std::clamp (i, 0, kNumPlaces - 1)];
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
    void prepare (int maxLen) { buf.assign ((size_t) std::max (2, maxLen + 1), 0.0f); w = 0; len = 1; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void setLength (int n) { len = std::clamp (n, 1, (int) buf.size() - 1); }
    inline float read() const { int r = w - len; if (r < 0) r += (int) buf.size(); return buf[(size_t) r]; }
    inline void write (float x) { buf[(size_t) w] = x; if (++w >= (int) buf.size()) w = 0; }
};

//==============================================================================
// Kleiner Raumhall: 8 gekoppelte Verzoegerungen (Householder), Daempfung im Kreis.
// Pegel so normiert, dass Rauschen ungefaehr mit gleichem RMS herauskommt.
class Reverb
{
public:
    static constexpr int N = 8;
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& d : line) d.prepare ((int) (0.045f * 2.2f * sr) + 8);
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
            const float sec = ms[i] * 0.001f * std::clamp (size, 0.4f, 2.2f);
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
struct Params
{
    int   placeA = 1, placeB = 0;
    float blend = 0.0f;        // 0 = Ort A, 1 = Ort B
    float distance = 0.3f;     // 0 = nah, 1 = weit
    float door = 1.0f;         // 0 = zu, 1 = offen
    float roomDb = 0.0f;       // Raumanteil der Quelle (Trim)
    float levelAmt = 1.0f;     // wie stark der Pegel mit Entfernung/Tuer sinkt (0..1)
    float outDb = 0.0f;
    float pan = 0.0f;          // -1 = links, 0 = Mitte, +1 = rechts (nur die Quelle, nicht der Raum)
    bool  pathDelay = false;
};

// Ein Ort mit eigener Filter-/Hall-Kette. Ortswechsel wird kurz aus- und eingeblendet (kein Knacken).
class Slot
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        rev.prepare (sr);
        for (auto& d : path) d.prepare ((int) (0.03f * sr));
        echo.prepare ((int) (0.2f * sr));
        current = -1;
        reset();
    }
    void reset()
    {
        for (auto& f : lp) f.reset();
        for (auto& d : path) d.reset();
        rev.reset(); echo.reset(); echoLp.reset();
    }
    // einmal pro Steuerblock
    void update (int wantedPlace, bool wantedDelay, float distance, float roomDb, float levelAmt)
    {
        wanted = wantedPlace; wantedDly = wantedDelay;
        if (current < 0) apply();
        const auto& p = place (current);
        fc = logLerp (p.fcNear, p.fcFar, distance);
        open = smooth01 ((fc - 12000.0f) / 8000.0f);      // nahe 20 kHz: Filter ganz umgehen
        for (auto& f : lp) f.set (fc, sr, 0.7071f);
        gain = dbToLin (lerp (p.gainNear, p.gainFar, distance) * levelAmt);
        wet = p.rt60 > 0.0f ? dbToLin (lerp (p.wetNear, p.wetFar, distance) + roomDb) : 0.0f;
        echoGain = p.echoMs > 0.0f ? dbToLin (lerp (p.echoNear, p.echoFar, distance) + roomDb) : 0.0f;
    }
    // dL/dR = Quelle (direkt), wL/wR = Raum der Quelle (Hall, Echo). Getrennt, damit nur die Quelle gepannt wird.
    inline void process (float inL, float inR, float& dL, float& dR, float& wL, float& wR)
    {
        if (wanted != current || wantedDly != dly)         // Wechsel: ausblenden, umschalten, einblenden
        {
            fade -= fadeStep();
            if (fade <= 0.0f) { fade = 0.0f; apply(); }
        }
        else if (hold > 0) --hold;                         // warten, bis die Weg-Verzoegerung gefuellt ist
        else if (fade < 1.0f) fade = std::min (1.0f, fade + fadeStep());

        if (dly)                                           // Laufzeit des Weges zuerst
        {
            path[0].write (inL); path[1].write (inR);
            inL = path[0].read(); inR = path[1].read();
        }
        float l = lp[0].lowpass (inL), r = lp[1].lowpass (inR);
        l += (inL - l) * open; r += (inR - r) * open;
        const float mono = 0.5f * (l + r);
        float wl = 0.0f, wr = 0.0f;
        if (wet > 0.0f)
        {
            float rl, rr; rev.process (mono, rl, rr);
            wl += rl * wet; wr += rr * wet;
        }
        if (echoGain > 0.0f)
        {
            const float e = echo.read();
            echo.write (mono + echoLp.lowpass (e) * 0.35f);
            wl += e * echoGain; wr += e * echoGain;
        }
        const float gg = gain * fade;
        dL = l * gg; dR = r * gg; wL = wl * gg; wR = wr * gg;
    }
private:
    float fadeStep() const { return 1.0f / (0.012f * sr); }
    void apply()
    {
        current = wanted; dly = wantedDly;
        const auto& p = place (current);
        reset();
        rev.set (std::max (0.1f, p.rt60), p.dampHz, p.size);
        for (auto& d : path) d.setLength (std::max (1, (int) (p.delayMs * 0.001f * sr)));
        echo.setLength (std::max (1, (int) (p.echoMs * 0.001f * sr)));
        echoLp.set (2500.0f, sr);
        hold = dly ? std::max (1, (int) (p.delayMs * 0.001f * sr)) : 0;
    }
    float sr = 48000.0f;
    int current = -1, wanted = 0, hold = 0;
    bool dly = false, wantedDly = false;
    float fade = 1.0f, fc = 20000.0f, open = 1.0f, gain = 1.0f, wet = 0.0f, echoGain = 0.0f;
    SVF lp[2]; Reverb rev; Delay path[2], echo; OnePole echoLp;
};

//==============================================================================
class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        a.prepare (sr); b.prepare (sr);
        smCoef = 1.0f - std::exp (-1.0f / (0.03f * sr / kBlock));   // 30 ms Glaettung
        first = true; counter = 0;
        reset();
    }
    void reset()
    {
        a.reset(); b.reset();
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

            const float inL = left[i], inR = right[i];
            float adl, adr, awl, awr, bdl, bdr, bwl, bwr;
            a.process (inL, inR, adl, adr, awl, awr);
            b.process (inL, inR, bdl, bdr, bwl, bwr);
            const float dl = adl + (bdl - adl) * sBlend, dr = adr + (bdr - adr) * sBlend;
            const float wl = awl + (bwl - awl) * sBlend, wr = awr + (bwr - awr) * sBlend;
            float l = dl * panL + wl;                       // Quelle gepannt, Raum bleibt stehen
            float r = dr * panR + wr;

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
            left[i] = l * outGain;
            if (right != left) right[i] = r * outGain;
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
        if (first) { sBlend = tBlend; sDist = tDist; sDoor = tDoor; sPan = tPan; first = false; }
        sPan += smCoef * (tPan - sPan);
        const float theta = (sPan + 1.0f) * kPi * 0.25f;   // Constant-Power, in der Mitte genau 0 dB
        panL = 1.41421356f * std::cos (theta);
        panR = 1.41421356f * std::sin (theta);
        sBlend += smCoef * (tBlend - sBlend);
        sDist += smCoef * (tDist - sDist);
        sDoor += smCoef * (tDoor - sDoor);
        const float lvl = std::clamp (p.levelAmt, 0.0f, 1.0f);
        a.update (p.placeA, p.pathDelay, sDist, p.roomDb, lvl);
        b.update (p.placeB, p.pathDelay, sDist, p.roomDb, lvl);

        // Tuer: zu = 650 Hz steil, dumpf, leise; offen = unveraendert
        const float closed = 1.0f - sDoor;
        const float fc = 650.0f * std::pow (20000.0f / 650.0f, std::pow (sDoor, 1.4f));
        for (int c = 0; c < 2; ++c)
        {
            doorLp[c * 2].set (fc, sr, 0.5412f);
            doorLp[c * 2 + 1].set (fc, sr, 1.3066f);
            doorHp[c].set (20.0f + 90.0f * closed, sr);
            doorBp[c].set (210.0f, sr, 2.3f);
        }
        doorBody = 0.35f * closed * closed;
        doorGain = dbToLin (-17.0f * std::pow (closed, 1.2f) * lvl);
        doorOpen = smooth01 ((sDoor - 0.9f) / 0.1f);
        outGain = dbToLin (p.outDb);
    }
    float sr = 48000.0f, smCoef = 1.0f;
    float sBlend = 0, sDist = 0.3f, sDoor = 1.0f, sPan = 0.0f, panL = 1.0f, panR = 1.0f;
    float doorBody = 0, doorGain = 1, doorOpen = 1, outGain = 1;
    bool first = true; int counter = 0;
    Slot a, b;
    SVF doorLp[4], doorBp[2]; OnePole doorHp[2];
};
} // namespace loc
