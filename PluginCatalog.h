#pragma once
#include "Common.h"

// Catalogo dei plug-in: nativi di Fender Studio Pro + quelli trovati nelle cartelle VST3/AU/CLAP
namespace mpm
{
enum class Tool { EQ, Compressor, DeEsser, Limiter, Reverb, Delay, Saturation, Imager, DynamicEQ, Transient, Gate, Multiband, NumTools };

class PluginCatalog : private juce::Thread
{
public:
    static PluginCatalog& get();

    // ogni processore chiama retain() quando nasce e release() quando muore:
    // il thread di scansione si ferma con l'ultima istanza, mai durante lo scaricamento della DLL
    void retain();
    void release();

    bool isReady() const noexcept               { return ready.load(); }
    juce::StringArray installedPlugins() const;
    juce::StringArray foundFor (Tool t) const;
    bool hasWaves() const;

    static juce::String nativeFor (Tool t);
    // "Pro EQ3 (nativo) · in alternativa: FabFilter Pro-Q 3"
    juce::String suggest (Tool t) const;

private:
    PluginCatalog();
    ~PluginCatalog() override;
    void run() override;
    static juce::Array<juce::File> searchFolders();

    mutable juce::CriticalSection lock;
    juce::StringArray installed;
    juce::StringArray found[(int) Tool::NumTools];   // risultati pre-calcolati alla fine della scansione
    bool waves = false;
    std::atomic<bool> ready { false };
    int users = 0;
    bool scanned = false;
};
} // namespace mpm
