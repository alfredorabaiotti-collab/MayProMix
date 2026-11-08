#include "Analyzer.h"

namespace mpm
{
static constexpr float kActiveGateLufs = -50.0f;

void Analyzer::prepare (double sampleRate)
{
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    subBlockLen = juce::jmax (64, (int) std::round (sr * 0.1));

    // Filtro K: shelf alto + passa-alto (coefficienti BS.1770 ricalcolati per qualsiasi frequenza)
    {
        const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / sr);
        const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
        const double a0 = 1.0 + K / Q + K * K;
        kShelf.b0 = (Vh + Vb * K / Q + K * K) / a0;
        kShelf.b1 = 2.0 * (K * K - Vh) / a0;
        kShelf.b2 = (Vh - Vb * K / Q + K * K) / a0;
        kShelf.a1 = 2.0 * (K * K - 1.0) / a0;
        kShelf.a2 = (1.0 - K / Q + K * K) / a0;
    }
    {
        const double f0 = 38.13547087602444, Q = 0.5003270373238773;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / sr);
        const double a0 = 1.0 + K / Q + K * K;
        kHighPass.b0 = 1.0; kHighPass.b1 = -2.0; kHighPass.b2 = 1.0;
        kHighPass.a1 = 2.0 * (K * K - 1.0) / a0;
        kHighPass.a2 = (1.0 - K / Q + K * K) / a0;
    }
    {
        // passa-basso Butterworth a 120 Hz per la correlazione dei bassi
        const double K = std::tan (juce::MathConstants<double>::pi * 120.0 / sr);
        const double Q = 0.7071067811865476, a0 = 1.0 + K / Q + K * K;
        lowL.b0 = K * K / a0; lowL.b1 = 2.0 * lowL.b0; lowL.b2 = lowL.b0;
        lowL.a1 = 2.0 * (K * K - 1.0) / a0;
        lowL.a2 = (1.0 - K / Q + K * K) / a0;
        lowR = lowL;
    }

    const int order = sr <= 50000.0 ? 12 : (sr <= 100000.0 ? 13 : 14);
    fftSize = 1 << order;
    fft = std::make_unique<juce::dsp::FFT> (order);
    fifo.assign ((size_t) fftSize, 0.0f);
    fftData.assign ((size_t) fftSize * 2, 0.0f);
    window.resize ((size_t) fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (fftSize - 1));

    reset();
}

void Analyzer::setPhaseOffset (juce::uint32 seed) noexcept
{
    phaseSeed = seed;
}

void Analyzer::reset()
{
    kShelf.clear(); kHighPass.clear(); lowL.clear(); lowR.clear();
    accK = accSq = accLR = accLL = accRR = lowLR = lowLL = lowRR = 0;
    accPeak = 0;
    subCount = subBlockLen > 0 ? (int) (phaseSeed % (juce::uint32) subBlockLen) : 0;   // sfasa gli FFT delle varie istanze
    ringK.fill (0); ringSq.fill (0); ringPeak.fill (0);
    ringPos = ringFilled = 0;
    histPower.fill (0); histCount.fill (0);
    crestSum = corrLR = corrLL = corrRR = cLowLR = cLowLL = cLowRR = 0;
    crestCount = activeBlocks = 0;
    shortLufs = momentaryLufs = recentPeak = maxPeak = kSilenceDb;
    std::fill (fifo.begin(), fifo.end(), 0.0f);
    fifoPos = 0;
    for (int b = 0; b < kNumBands; ++b) longBandPow[b] = shortBandPow[b] = 0;
    longBandCount = 0;
}

bool Analyzer::process (const float* const* ch, int numChannels, int numSamples)
{
    if (numChannels <= 0 || numSamples <= 0 || fft == nullptr) return false;
    numCh = juce::jmin (2, numChannels);
    const float* L = ch[0];
    const float* R = numCh > 1 ? ch[1] : ch[0];
    bool completed = false;

    for (int i = 0; i < numSamples; ++i)
    {
        double l = L[i], r = R[i];
        if (! std::isfinite (l)) l = 0.0;   // un campione NaN/Inf di un altro plug-in non deve rovinare l'analisi
        if (! std::isfinite (r)) r = 0.0;

        // loudness K-pesata
        const double kl = kHighPass.run (kShelf.run (l, 0), 0);
        accK += kl * kl;
        if (numCh > 1)
        {
            const double kr = kHighPass.run (kShelf.run (r, 1), 1);
            accK += kr * kr;
        }

        const double m = numCh > 1 ? 0.5 * (l + r) : l;
        accSq += numCh > 1 ? 0.5 * (l * l + r * r) : l * l;
        accPeak = juce::jmax (accPeak, (float) std::abs (l), (float) std::abs (r));
        accLR += l * r; accLL += l * l; accRR += r * r;
        const double ll = lowL.run (l, 0), lr = lowR.run (r, 0);
        lowLR += ll * lr; lowLL += ll * ll; lowRR += lr * lr;

        fifo[(size_t) fifoPos] = (float) m;
        fifoPos = (fifoPos + 1) % fftSize;

        if (++subCount >= subBlockLen)
        {
            finishSubBlock();
            completed = true;
        }
    }
    return completed;
}

void Analyzer::finishSubBlock()
{
    const double n = (double) juce::jmax (1, subCount);
    ringK[(size_t) ringPos] = accK / n;     // somma dei quadrati medi dei canali
    ringSq[(size_t) ringPos] = accSq / n;
    ringPeak[(size_t) ringPos] = accPeak;
    ringPos = (ringPos + 1) % kRing;
    ringFilled = juce::jmin (kRing, ringFilled + 1);

    auto meanOfLast = [this] (const std::array<double, kRing>& ring, int count)
    {
        count = juce::jmin (count, ringFilled);
        double s = 0;
        for (int k = 1; k <= count; ++k) s += ring[(size_t) ((ringPos - k + kRing) % kRing)];
        return count > 0 ? s / count : 0.0;
    };

    const double momPow = meanOfLast (ringK, 4);
    const double shortPow = meanOfLast (ringK, 30);
    momentaryLufs = juce::jlimit (kSilenceDb, 30.0f, finiteOr ((float) (-0.691 + 10.0 * std::log10 (momPow + 1.0e-20)), kSilenceDb));
    shortLufs     = juce::jlimit (kSilenceDb, 30.0f, finiteOr ((float) (-0.691 + 10.0 * std::log10 (shortPow + 1.0e-20)), kSilenceDb));

    const float peakDb = juce::Decibels::gainToDecibels (accPeak, kSilenceDb);
    recentPeak = juce::jmax (peakDb, recentPeak - 1.5f);
    maxPeak = juce::jmax (maxPeak, peakDb);

    if (momentaryLufs > -70.0f)
    {
        const int bin = juce::jlimit (0, kHistBins - 1, (int) ((momentaryLufs + 70.0f) * 10.0f));
        histPower[(size_t) bin] += momPow;
        histCount[(size_t) bin] += 1;
    }

    float bandsNow[kNumBands];
    runSpectrum (bandsNow);
    for (int b = 0; b < kNumBands; ++b)
        shortBandPow[b] = 0.75 * shortBandPow[b] + 0.25 * (double) dbToPow (bandsNow[b]);

    if (momentaryLufs > kActiveGateLufs)
    {
        ++activeBlocks;

        // fattore di cresta sugli ultimi 400 ms
        float pk = 0;
        for (int k = 1; k <= juce::jmin (4, ringFilled); ++k)
            pk = juce::jmax (pk, ringPeak[(size_t) ((ringPos - k + kRing) % kRing)]);
        const double ms = meanOfLast (ringSq, 4);
        if (ms > 1.0e-12 && pk > 0)
        {
            crestSum += 20.0 * std::log10 (pk / std::sqrt (ms));
            ++crestCount;
        }

        corrLR += accLR; corrLL += accLL; corrRR += accRR;
        cLowLR += lowLR; cLowLL += lowLL; cLowRR += lowRR;

        for (int b = 0; b < kNumBands; ++b)
            longBandPow[b] += (double) dbToPow (bandsNow[b]);
        ++longBandCount;
    }

    accK = accSq = accLR = accLL = accRR = lowLR = lowLL = lowRR = 0;
    accPeak = 0;
    subCount = 0;
}

void Analyzer::runSpectrum (float (&bandsOut)[kNumBands])
{
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = fifo[(size_t) ((fifoPos + i) % fftSize)] * window[(size_t) i];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    fft->performFrequencyOnlyForwardTransform (fftData.data(), true);

    const double binHz = sr / fftSize;
    const double norm = 4.0 / ((double) fftSize * (double) fftSize);   // Hann ~ 0.5 di guadagno

    for (int b = 0; b < kNumBands; ++b)
    {
        const double lo = kBandEdges[b];
        const double hi = juce::jmin ((double) kBandEdges[b + 1], sr * 0.5);
        double p = 0;
        if (hi > lo)
        {
            const int k0 = juce::jmax (1, (int) std::ceil (lo / binHz));
            const int k1 = juce::jmin (fftSize / 2, (int) std::floor (hi / binHz));
            for (int k = k0; k <= k1; ++k)
            {
                const double mag = fftData[(size_t) k];
                p += mag * mag;
            }
            p = p * norm / std::log2 (hi / lo);   // potenza per ottava
        }
        bandsOut[b] = powToDb (p);
    }
}

void Analyzer::getResults (AnalysisResults& r) const
{
    r.shortLufs = shortLufs;
    r.momentaryLufs = momentaryLufs;
    r.recentPeakDb = recentPeak;
    r.maxPeakDb = maxPeak;
    r.activeSeconds = (float) activeBlocks * 0.1f;
    r.crestDb = crestCount > 0 ? finiteOr ((float) (crestSum / crestCount), 0.0f) : 0.0f;

    auto corr = [] (double lr, double ll, double rr)
    {
        return ll * rr > 1.0e-18 ? (float) juce::jlimit (-1.0, 1.0, lr / std::sqrt (ll * rr)) : 1.0f;
    };
    r.correlation = corr (corrLR, corrLL, corrRR);
    r.lowCorrelation = corr (cLowLR, cLowLL, cLowRR);

    // loudness integrata con gate assoluto (-70) e relativo (-10 LU)
    double sumP = 0; double cnt = 0;
    for (int i = 0; i < kHistBins; ++i) { sumP += histPower[(size_t) i]; cnt += histCount[(size_t) i]; }
    r.integratedLufs = kSilenceDb;
    if (cnt > 0)
    {
        const double ungated = juce::jlimit (-100.0, 30.0, -0.691 + 10.0 * std::log10 (sumP / cnt + 1.0e-20));
        const int gateBin = juce::jlimit (0, kHistBins - 1, (int) ((ungated - 10.0 + 70.0) * 10.0));
        double p2 = 0, c2 = 0;
        for (int i = gateBin; i < kHistBins; ++i) { p2 += histPower[(size_t) i]; c2 += histCount[(size_t) i]; }
        if (c2 > 0) r.integratedLufs = juce::jlimit (kSilenceDb, 30.0f, finiteOr ((float) (-0.691 + 10.0 * std::log10 (p2 / c2 + 1.0e-20)), kSilenceDb));
    }

    for (int b = 0; b < kNumBands; ++b)
    {
        r.bandsLong[b] = longBandCount > 0 ? powToDb (longBandPow[b] / longBandCount) : kSilenceDb;
        r.bandsShort[b] = powToDb (shortBandPow[b]);
    }
}
} // namespace mpm
