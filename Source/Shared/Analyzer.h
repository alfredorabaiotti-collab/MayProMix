#pragma once
#include "Common.h"
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

// Analisi audio in tempo reale: loudness ITU-R BS.1770 (LUFS), picchi, fattore di cresta,
// correlazione stereo e spettro a 8 bande. Lavora sul thread audio senza allocare memoria.
namespace mpm
{
struct AnalysisResults
{
    float shortLufs = kSilenceDb, momentaryLufs = kSilenceDb, integratedLufs = kSilenceDb;
    float recentPeakDb = kSilenceDb, maxPeakDb = kSilenceDb, crestDb = 0.0f;
    float correlation = 1.0f, lowCorrelation = 1.0f, activeSeconds = 0.0f;
    float bandsLong[kNumBands] {}, bandsShort[kNumBands] {};
};

class Analyzer
{
public:
    void prepare (double sampleRate);
    void reset();
    void setPhaseOffset (juce::uint32 seed) noexcept;

    // Ritorna true quando e' stato completato almeno un blocco da 100 ms
    bool process (const float* const* channels, int numChannels, int numSamples);
    void getResults (AnalysisResults& r) const;

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1[2] {}, z2[2] {};
        inline double run (double x, int ch) noexcept
        {
            const double y = b0 * x + z1[ch];
            z1[ch] = b1 * x - a1 * y + z2[ch];
            z2[ch] = b2 * x - a2 * y;
            return y;
        }
        void clear() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    };

    void finishSubBlock();
    void runSpectrum (float (&bandsOut)[kNumBands]);

    double sr = 48000.0;
    int subBlockLen = 4800, subCount = 0, numCh = 2;
    juce::uint32 phaseSeed = 0;
    Biquad kShelf, kHighPass, lowL, lowR;

    // accumulatori del blocco da 100 ms
    double accK = 0, accSq = 0, accLR = 0, accLL = 0, accRR = 0, lowLR = 0, lowLL = 0, lowRR = 0;
    float accPeak = 0;

    static constexpr int kRing = 30;
    std::array<double, kRing> ringK {}, ringSq {};
    std::array<float, kRing> ringPeak {};
    int ringPos = 0, ringFilled = 0;

    // istogramma per la loudness integrata (gating BS.1770)
    static constexpr int kHistBins = 800;   // da -70 a +10 LUFS in passi da 0.1
    std::array<double, kHistBins> histPower {};
    std::array<juce::uint32, kHistBins> histCount {};

    double crestSum = 0, corrLR = 0, corrLL = 0, corrRR = 0, cLowLR = 0, cLowLL = 0, cLowRR = 0;
    int crestCount = 0, activeBlocks = 0;
    float shortLufs = kSilenceDb, momentaryLufs = kSilenceDb, recentPeak = kSilenceDb, maxPeak = kSilenceDb;

    // spettro
    std::unique_ptr<juce::dsp::FFT> fft;
    int fftSize = 4096, fifoPos = 0;
    std::vector<float> fifo, fftData, window;
    double longBandPow[kNumBands] {};
    double shortBandPow[kNumBands] {};
    int longBandCount = 0;
};
} // namespace mpm
