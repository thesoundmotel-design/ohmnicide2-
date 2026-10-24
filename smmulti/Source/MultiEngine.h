#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Distorsión multibanda (3 bandas, crossover Linkwitz-Riley 4º orden) con modelo de
// distorsión por banda y generador de feedback. DSP original, sin dependencias de JUCE.
class MultiEngine
{
public:
    static constexpr int numModels = 8;

    struct Params
    {
        float xo1 = 200.0f, xo2 = 2500.0f;                 // Hz
        int   model[3]   = { 0, 2, 1 };
        float drive[3]   = { 0.35f, 0.35f, 0.35f };         // 0..1
        float levelDb[3] = { 0.0f, 0.0f, 0.0f };
        float fb = 0.0f;                                    // 0..0.95
        float fbFreq = 220.0f;                              // Hz
        float mix = 1.0f;                                   // 0..1
        float outDb = 0.0f;
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        fbSize = (int) std::ceil (sr / 30.0f) + 8;
        for (auto& c : chans) c.reset (fbSize);
        snap = true;
        tau = 1.0f - std::exp (-1.0f / (0.02f * sr));
        dcR = 1.0f - 6.28318530718f * 10.0f / sr;
        fbLpA = 1.0f - std::exp (-6.28318530718f * 6000.0f / sr);
        coefCounter = 0;
    }

    void setParams (const Params& p) { target = p; }

    void process (float* const* io, int numCh, int n)
    {
        numCh = std::min (numCh, 2);
        if (numCh <= 0) return;
        if (snap) { cur = target; snap = false; updateCoefs(); }

        for (int i = 0; i < n; ++i)
        {
            // suavizado
            cur.xo1 += (target.xo1 - cur.xo1) * tau;
            cur.xo2 += (target.xo2 - cur.xo2) * tau;
            for (int b = 0; b < 3; ++b)
            {
                cur.model[b] = target.model[b];
                cur.drive[b] += (target.drive[b] - cur.drive[b]) * tau;
                cur.levelDb[b] += (target.levelDb[b] - cur.levelDb[b]) * tau;
            }
            cur.fb += (target.fb - cur.fb) * tau;
            cur.fbFreq += (target.fbFreq - cur.fbFreq) * tau;
            cur.mix += (target.mix - cur.mix) * tau;
            cur.outDb += (target.outDb - cur.outDb) * tau;

            if (++coefCounter >= 32) { coefCounter = 0; updateCoefs(); }

            float pre[3], lvl[3];
            for (int b = 0; b < 3; ++b)
            {
                pre[b] = std::pow (10.0f, cur.drive[b] * 2.0f);          // 0..+40 dB
                lvl[b] = std::pow (10.0f, cur.levelDb[b] * 0.05f);
            }
            const float outG = std::pow (10.0f, cur.outDb * 0.05f);
            const float fbDelay = std::min ((float) (fbSize - 4), std::max (2.0f, sr / std::max (20.0f, cur.fbFreq)));
            const float fbAmt = cur.fb;
            const float dryG = std::cos (cur.mix * 1.5707963f), wetG = std::sin (cur.mix * 1.5707963f);

            for (int c = 0; c < numCh; ++c)
            {
                Chan& ch = chans[c];
                const float in = io[c][i];

                // --- crossover LR4 ---
                float low  = ch.lp1b.run (cLp1, ch.lp1a.run (cLp1, in));
                float rest = ch.hp1b.run (cHp1, ch.hp1a.run (cHp1, in));
                float mid  = ch.lp2b.run (cLp2, ch.lp2a.run (cLp2, rest));
                float high = ch.hp2b.run (cHp2, ch.hp2a.run (cHp2, rest));
                // compensación de fase de la banda grave (all-pass al cruce 2)
                low = ch.lpcb.run (cLp2, ch.lpca.run (cLp2, low)) + ch.hpcb.run (cHp2, ch.hpca.run (cHp2, low));

                float bands[3] = { low, mid, high };
                float sum = 0.0f;
                for (int b = 0; b < 3; ++b)
                {
                    float y = shape (cur.model[b], bands[b], cur.drive[b], pre[b], ch.crush[b]);
                    // bloqueador de DC (los modelos asimétricos generan DC)
                    const float dc = y - ch.dcX[b] + dcR * ch.dcY[b];
                    ch.dcX[b] = y; ch.dcY[b] = dc;
                    sum += dc * lvl[b];
                }

                // --- generador de feedback (peine resonante con saturación) ---
                float d = ch.readFb (fbDelay);
                ch.fbLp += fbLpA * (d - ch.fbLp);
                const float fbSig = std::tanh (ch.fbLp * fbAmt);
                float proc = sum + fbSig;
                ch.writeFb (proc);

                if (! std::isfinite (proc))
                {
                    ch.reset (fbSize);
                    proc = 0.0f;
                }

                float out = (in * dryG + proc * wetG) * outG;
                io[c][i] = softLimit (out);
            }
        }
    }

private:
    struct Coef { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

    struct Biquad
    {
        float z1 = 0, z2 = 0;
        float run (const Coef& c, float x)
        {
            const float y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
    };

    struct Crush { float held = 0; int cnt = 0; };

    struct Chan
    {
        Biquad lp1a, lp1b, hp1a, hp1b, lp2a, lp2b, hp2a, hp2b, lpca, lpcb, hpca, hpcb;
        Crush crush[3];
        float dcX[3] = { 0, 0, 0 }, dcY[3] = { 0, 0, 0 };
        std::vector<float> fbBuf; int w = 0; float fbLp = 0;

        void reset (int n)
        {
            *this = Chan();
            fbBuf.assign ((size_t) n, 0.0f);
        }
        void writeFb (float x) { fbBuf[(size_t) w] = x; if (++w >= (int) fbBuf.size()) w = 0; }
        float readFb (float d) const
        {
            const int n = (int) fbBuf.size();
            double rp = (double) w - (double) d;
            if (rp < 0.0) rp += (double) n;
            const int i0 = (int) rp; const float fr = (float) (rp - (double) i0);
            int i1 = i0 + 1; if (i1 >= n) i1 = 0;
            return fbBuf[(size_t) i0] * (1.0f - fr) + fbBuf[(size_t) i1] * fr;
        }
    };

    static float softLimit (float x)
    {
        const float a = std::fabs (x);
        if (a < 0.9f) return x;
        const float y = 0.9f + 0.1f * std::tanh ((a - 0.9f) * 10.0f);
        return x < 0 ? -y : y;
    }

    static float shape (int model, float x, float drive, float g, Crush& cr)
    {
        switch (model)
        {
            case 0:  return std::tanh (g * x);                                           // Soft Clip
            case 1:  return std::max (-1.0f, std::min (1.0f, g * x));                    // Hard Clip
            case 2:  return std::tanh (g * x + 0.3f) - std::tanh (0.3f);                 // Tube (asimétrico)
            case 3:  { const float u = g * x * 0.25f + 0.25f;                            // Fold (triangular)
                       return 4.0f * std::fabs (u - std::floor (u + 0.5f)) - 1.0f; }
            case 4:  return std::tanh (g * std::fabs (x));                               // Rectify
            case 5:  { if (cr.cnt <= 0)                                                  // Bit Crush + decimación
                       {
                           const float bits = 16.0f - drive * 13.0f;
                           const float lev = std::pow (2.0f, bits - 1.0f);
                           cr.held = std::round (x * 4.0f * lev) / (lev * 4.0f) * 2.0f;
                           cr.cnt = 1 + (int) (drive * 15.0f);
                       }
                       --cr.cnt; return cr.held; }
            case 6:  return std::sin (std::min (g, 30.0f) * x);                          // Sine shaper
            default: return x > 0 ? std::tanh (g * x)                                    // Fuzz (asimétrico)
                                  : 0.7f * std::max (-1.0f, std::min (1.0f, 0.5f * g * x));
        }
    }

    static void butter (Coef& c, float f, float sr, bool highpass)
    {
        f = std::min (std::max (f, 20.0f), 0.45f * sr);
        const float w0 = 6.28318530718f * f / sr, cs = std::cos (w0), al = std::sin (w0) / (2.0f * 0.70710678f);
        const float a0 = 1.0f + al;
        if (! highpass) { c.b0 = (1 - cs) * 0.5f; c.b1 = 1 - cs;  c.b2 = (1 - cs) * 0.5f; }
        else            { c.b0 = (1 + cs) * 0.5f; c.b1 = -(1 + cs); c.b2 = (1 + cs) * 0.5f; }
        c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
        c.a1 = -2.0f * cs / a0; c.a2 = (1.0f - al) / a0;
    }

    void updateCoefs()
    {
        const float f1 = std::min (std::max (cur.xo1, 30.0f), 4000.0f);
        const float f2 = std::max (cur.xo2, f1 * 1.5f);
        butter (cLp1, f1, sr, false); butter (cHp1, f1, sr, true);
        butter (cLp2, f2, sr, false); butter (cHp2, f2, sr, true);
    }

    float sr = 44100.0f, tau = 0.01f, dcR = 0.999f, fbLpA = 0.5f;
    int fbSize = 0, coefCounter = 0;
    bool snap = true;
    Params cur, target;
    Coef cLp1, cHp1, cLp2, cHp2;
    Chan chans[2];
};
