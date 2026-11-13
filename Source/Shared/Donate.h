#pragma once
#include "Theme.h"
#include "SharedBus.h"
#include "MayWelcome.h"

// Finestra di benvenuto (prima apertura) e pulsante "Dona" con i colori di May Pro Mix
namespace mpm::ui
{
inline may::WelcomeStyle welcomeStyle()
{
    may::WelcomeStyle s;
    s.panel = C::panel; s.border = C::line; s.text = C::text; s.dim = C::dim;
    s.accent = C::teal; s.accentText = juce::Colour (0xff0f1a1c);
    return s;
}

inline std::function<bool()> welcomeEnglish() { return [] { return mpm::SharedBus::get().english(); }; }
inline std::function<void (bool)> welcomeSetEnglish() { return [] (bool en) { mpm::SharedBus::get().setEnglish (en); }; }
} // namespace mpm::ui
