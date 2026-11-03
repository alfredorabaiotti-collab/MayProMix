#include "SharedBus.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
#else
 #include <sys/mman.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <unistd.h>
#endif

namespace mpm
{
static constexpr juce::uint32 kMagic = 0x4d50524d; // "MPRM"
static constexpr juce::uint32 kVersion = 4;

SharedBus& SharedBus::get()
{
    // creato una volta e mai distrutto: evita crash all'uscita se il thread audio scrive ancora
    static SharedBus* instance = new SharedBus();
    return *instance;
}

// nome della memoria condivisa legato a versione e dimensione: due build diverse non si mescolano mai
[[maybe_unused]] static juce::String busName()
{
    return "MayProMixBus_v" + juce::String ((int) kVersion) + "_" + juce::String ((int) sizeof (BusLayout));
}

SharedBus::SharedBus()
{
    const auto size = sizeof (BusLayout);

   #if JUCE_WINDOWS
    const juce::String wname ("Local\\" + busName());
    if (auto h = CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, (DWORD) size, wname.toWideCharPointer()))
    {
        if (auto* p = MapViewOfFile (h, FILE_MAP_ALL_ACCESS, 0, 0, size))
        {
            layout = static_cast<BusLayout*> (p);
            nativeHandle = h;
            interProcess = true;
        }
        else
        {
            CloseHandle (h);
        }
    }
   #else
    const auto pname = "/MPM" + juce::String ((int) kVersion) + "_" + juce::String ((int) sizeof (BusLayout));   // max 31 caratteri su macOS
    const int fd = shm_open (pname.toRawUTF8(), O_RDWR | O_CREAT, 0666);
    if (fd >= 0)
    {
        struct stat st {};
        bool ok = fstat (fd, &st) == 0;
        if (ok && st.st_size == 0)
            ok = ftruncate (fd, (off_t) size) == 0;
        else if (ok)
            ok = (size_t) st.st_size >= size;

        if (ok)
        {
            void* p = mmap (nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (p != MAP_FAILED)
            {
                layout = static_cast<BusLayout*> (p);
                interProcess = true;
            }
        }
        close (fd);
    }
   #endif

    if (layout == nullptr)
    {
        // Ripiego: memoria locale al plug-in
        static BusLayout fallback {};
        layout = &fallback;
    }

    initialiseIfNeeded();
}

// lock con timeout: se un altro processo si blocca, non ci blocchiamo anche noi
struct TryLock
{
    explicit TryLock (juce::InterProcessLock& l) : lock (l), held (l.enter (500)) {}
    ~TryLock() { if (held) lock.exit(); }
    juce::InterProcessLock& lock;
    bool held;
};

void SharedBus::initialiseIfNeeded()
{
    TryLock sl (lock);
    auto& h = layout->header;
    if (h.magic != kMagic || h.version != kVersion)
    {
        std::memset (layout, 0, sizeof (BusLayout));
        h.magic = kMagic;
        h.version = kVersion;
        h.genre = 0;
        h.loudTarget = targetGenre;
        const auto saved = settings()->getValue ("language");
        h.language = saved.isNotEmpty() ? (saved == "en" ? 1 : 0)
                                        : (juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("it") ? 0 : 1);
        h.masterIntegrated = h.masterShort = h.masterMomentary = kSilenceDb;
    }
}

std::unique_ptr<juce::PropertiesFile> SharedBus::settings()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "MayProMix";
    o.folderName = "May Studio Recording";
    o.filenameSuffix = ".settings";
    o.osxLibrarySubFolder = "Application Support";
    return std::make_unique<juce::PropertiesFile> (o);
}

void SharedBus::setEnglish (bool en)
{
    layout->header.language = en ? 1 : 0;
    auto s = settings();
    s->setValue ("language", en ? "en" : "it");
    s->saveIfNeeded();
}

int SharedBus::claimSlot (juce::uint32 ownerId)
{
    TryLock sl (lock);
    const auto t = now();

    for (int i = 0; i < kMaxSlots; ++i)
    {
        auto& s = layout->slots[i];
        if (s.ownerId == 0 || t - s.lastSeenMs > 30000 || s.lastSeenMs > t + 60000)
        {
            std::memset (&s, 0, sizeof (SlotData));
            s.ownerId = ownerId;
            s.lastSeenMs = t;
            s.role = (int) Role::Other;
            s.resetAck = layout->header.resetCounter;
            s.shortLufs = s.momentaryLufs = s.integratedLufs = kSilenceDb;
            s.recentPeakDb = s.maxPeakDb = kSilenceDb;
            for (int b = 0; b < kNumBands; ++b)
                s.bandsLong[b] = s.bandsShort[b] = kSilenceDb;
            return i;
        }
    }
    return -1;
}

DoneMark* SharedBus::marksFor (int slot) noexcept
{
    if (slot < 0) return layout->header.mixDone;
    if (slot >= kMaxSlots) return nullptr;
    return layout->slots[slot].done;
}

juce::uint32 SharedBus::epochFor (int slot) const noexcept
{
    if (slot < 0) return layout->header.resetCounter;
    if (slot >= kMaxSlots) return 0;
    return layout->slots[slot].resetEpoch;
}

void SharedBus::setDone (int slot, const juce::String& key, const juce::String& title, bool done)
{
    auto* marks = marksFor (slot);
    if (marks == nullptr || key.isEmpty()) return;
    TryLock sl (lock);

    for (int i = 0; i < kMaxDone; ++i)
        if (marks[i].used && key == juce::String::fromUTF8 (marks[i].key, (int) strnlen (marks[i].key, sizeof (marks[i].key))))
        {
            if (! done) std::memset (&marks[i], 0, sizeof (DoneMark));
            return;
        }
    if (! done) return;

    // posto libero, altrimenti sostituisco il piu' vecchio
    int target = 0;
    for (int i = 0; i < kMaxDone; ++i)
    {
        if (! marks[i].used) { target = i; break; }
        if (marks[i].epoch < marks[target].epoch) target = i;
    }
    auto& m = marks[target];
    std::memset (&m, 0, sizeof (DoneMark));
    key.copyToUTF8 (m.key, sizeof (m.key) - 1);
    title.copyToUTF8 (m.title, sizeof (m.title) - 1);
    m.epoch = epochFor (slot);
    m.used = 1;
}

void SharedBus::releaseSlot (int index, juce::uint32 ownerId)
{
    if (index < 0 || index >= kMaxSlots) return;
    TryLock sl (lock);
    auto& s = layout->slots[index];
    if (s.ownerId == ownerId)
    {
        s.ownerId = 0;
        s.lastSeenMs = 0;
    }
}
} // namespace mpm
