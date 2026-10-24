#include "../Source/MultiEngine.h"
#include <cstdio>
#include <random>

static double rmsOfSine (double f, float amp, double sr, MultiEngine::Params p)
{
    MultiEngine e; e.prepare (sr); e.setParams (p);
    const int N = (int) (sr * 1.0);
    std::vector<float> x ((size_t) N);
    for (int i = 0; i < N; ++i) x[(size_t) i] = amp * (float) std::sin (6.283185307 * f * i / sr);
    float* io[1] = { x.data() };
    e.process (io, 1, N);
    double s = 0; int c = 0;
    for (int i = N / 2; i < N; ++i) { s += x[(size_t) i] * x[(size_t) i]; ++c; }
    return std::sqrt (s / c) / (amp / std::sqrt (2.0));
}

int main()
{
    const double sr = 48000.0;
    int bad = 0;

    // 1) Suma plana del crossover (señal débil, drive 0, nivel 0 dB, soft clip lineal)
    MultiEngine::Params p; p.model[0] = p.model[1] = p.model[2] = 0;
    for (int b = 0; b < 3; ++b) p.drive[b] = 0.0f;
    double worst = 0;
    for (double f : { 60.0, 100.0, 150.0, 200.0, 300.0, 600.0, 1200.0, 2000.0, 2500.0, 3500.0, 6000.0, 12000.0 })
    {
        const double r = rmsOfSine (f, 0.01f, sr, p);
        const double db = 20 * std::log10 (r);
        worst = std::max (worst, std::fabs (db));
        std::printf ("  %6.0f Hz: %+.2f dB\n", f, db);
    }
    std::printf ("Desviación máxima del crossover: %.2f dB\n", worst);
    if (worst > 1.0) ++bad;

    // 2) Estabilidad con todos los modelos, drive máximo, feedback máximo
    for (int m0 = 0; m0 < MultiEngine::numModels; ++m0)
        for (int m1 = 0; m1 < MultiEngine::numModels; ++m1)
        {
            MultiEngine e; e.prepare (sr);
            MultiEngine::Params q;
            q.model[0] = m0; q.model[1] = m1; q.model[2] = (m0 + m1) % 8;
            for (int b = 0; b < 3; ++b) { q.drive[b] = 1.0f; q.levelDb[b] = 12.0f; }
            q.fb = 0.95f; q.fbFreq = 80.0f; q.mix = 1.0f; q.outDb = 12.0f;
            e.setParams (q);
            std::mt19937 rng (3); std::uniform_real_distribution<float> u (-1.0f, 1.0f);
            std::vector<float> L (512), R (512);
            float maxOut = 0; bool finite = true; double energy = 0;
            for (int b = 0; b < (int) (sr * 6 / 512); ++b)
            {
                for (int i = 0; i < 512; ++i) { L[(size_t) i] = (b % 50 < 10) ? u (rng) : 0.0f; R[(size_t) i] = L[(size_t) i]; }
                if (b == 100) { q.xo1 = 800; q.xo2 = 900; q.fbFreq = 1500; e.setParams (q); }
                float* io[2] = { L.data(), R.data() };
                e.process (io, 2, 512);
                for (int i = 0; i < 512; ++i)
                { if (! std::isfinite (L[(size_t) i])) finite = false; maxOut = std::max (maxOut, std::fabs (L[(size_t) i])); energy += L[(size_t) i] * L[(size_t) i]; }
            }
            if (! finite || maxOut > 1.01f || energy < 1e-6) { std::printf ("PROBLEMA m0=%d m1=%d max=%.3f finite=%d\n", m0, m1, maxOut, finite); ++bad; }
        }
    std::printf ("Problemas: %d\n", bad);
    return bad;
}
