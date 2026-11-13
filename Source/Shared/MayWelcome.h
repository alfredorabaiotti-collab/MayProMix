#pragma once
#include <juce_gui_extra/juce_gui_extra.h>

// Finestra di benvenuto dei plug-in May Studio Recording: si apre da sola alla prima apertura
// e si riapre con il piccolo pulsante "Dona" in basso a destra. Italiano e inglese.
// File autonomo: e' identico in tutti i plug-in (May Pro Sound Designer, May Pro Mix, May Studio Midi Drummer).
namespace may
{
struct WelcomeStyle
{
    juce::Colour backdrop { 0xa0000000 }, panel { 0xff252e39 }, border { 0xff36414f };
    juce::Colour text { 0xffdfe7ee }, dim { 0xff8e9bab }, accent { 0xfff2a14b }, accentText { 0xff1a1208 };
};

// Aspetto dei pulsanti della finestra, indipendente dal LookAndFeel del plug-in che la ospita
class WelcomeButtonLook : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bg, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        auto c = bg;
        if (down) c = c.darker (0.15f); else if (over) c = c.brighter (0.08f);
        g.setColour (c);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (c.brighter (0.25f).withAlpha (0.6f));
        g.drawRoundedRectangle (r, 8.0f, 1.0f);
    }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (13.0f, juce::Font::bold)); }
};

class Welcome : public juce::Component
{
public:
    static constexpr const char* kPayPalEmail = "alfredorabaiotti@gmail.com";

    // productKey: nome breve senza spazi (es. "MayProSoundDesigner"), productName: nome mostrato
    Welcome (juce::String productKey, juce::String productName, std::function<bool()> isEnglish,
             std::function<void (bool)> setEnglish = {}, WelcomeStyle s = {})
        : key (std::move (productKey)), name (std::move (productName)),
          english (std::move (isEnglish)), changeLanguage (std::move (setEnglish)), st (s)
    {
        for (auto* b : { &donateBtn, &copyBtn, &closeBtn, &langBtn })
        {
            addAndMakeVisible (b);
            b->setLookAndFeel (&buttonLook);
            b->setColour (juce::TextButton::buttonColourId, st.panel.brighter (0.08f));
            b->setColour (juce::TextButton::textColourOffId, st.text);
        }
        donateBtn.setColour (juce::TextButton::buttonColourId, st.accent);
        donateBtn.setColour (juce::TextButton::textColourOffId, st.accentText);
        langBtn.setVisible ((bool) changeLanguage);

        donateBtn.onClick = []
        {
            juce::URL ("https://www.paypal.com/donate/?business=alfredorabaiotti%40gmail.com&currency_code=EUR").launchInDefaultBrowser();
        };
        copyBtn.onClick = [this]
        {
            juce::SystemClipboard::copyTextToClipboard (kPayPalEmail);
            copied = true;
            refreshTexts();
        };
        closeBtn.onClick = [this] { close(); };
        langBtn.onClick = [this]
        {
            if (changeLanguage) changeLanguage (! en());
            refreshTexts();
            if (onLanguageChanged) onLanguageChanged();
        };
        setInterceptsMouseClicks (true, true);
        setVisible (false);
        refreshTexts();
    }

    ~Welcome() override
    {
        for (auto* b : { &donateBtn, &copyBtn, &closeBtn, &langBtn }) b->setLookAndFeel (nullptr);
    }

    std::function<void()> onClose, onLanguageChanged;

    static std::unique_ptr<juce::PropertiesFile> store()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "MayStudioWelcome";
        o.filenameSuffix = ".settings";
        o.folderName = "May Studio Recording";
        o.osxLibrarySubFolder = "Application Support";
        o.storageFormat = juce::PropertiesFile::storeAsXML;
        return std::make_unique<juce::PropertiesFile> (o);
    }

    // Lingua della finestra per i plug-in che non hanno un proprio selettore IT / EN
    static bool storedEnglish() { return store()->getValue ("welcomeLanguage", "it") == "en"; }
    static void storeEnglish (bool en)
    {
        auto s = store();
        s->setValue ("welcomeLanguage", en ? "en" : "it");
        s->saveIfNeeded();
    }

    bool alreadySeen() const { return store()->getBoolValue ("welcomeSeen_" + key, false); }

    // Da chiamare dal costruttore dell'editor: alla prima apertura mostra la finestra
    void showIfFirstTime() { if (! alreadySeen()) open(); }

    void open()
    {
        copied = false;
        refreshTexts();
        setVisible (true);
        toFront (false);
    }

    void refreshTexts()
    {
        donateBtn.setButtonText (t ("Dona con PayPal", "Donate with PayPal"));
        copyBtn.setButtonText (copied ? t ("Indirizzo copiato", "Address copied") : t ("Copia l'indirizzo PayPal", "Copy PayPal address"));
        closeBtn.setButtonText (t ("Continua", "Continue"));
        langBtn.setButtonText (en() ? "IT" : "EN");
        repaint();
    }

    void resized() override
    {
        card = getLocalBounds().withSizeKeepingCentre (juce::jmin (560, getWidth() - 40), juce::jmin (320, getHeight() - 40));
        auto b = card.reduced (26, 22);
        langBtn.setBounds (b.getRight() - 40, b.getY(), 40, 24);
        auto row = b.removeFromBottom (34);
        closeBtn.setBounds (row.removeFromRight (110));
        row.removeFromRight (10);
        donateBtn.setBounds (row.removeFromRight (170));
        row.removeFromRight (10);
        copyBtn.setBounds (row.removeFromRight (juce::jmin (190, row.getWidth())));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (st.backdrop);
        auto r = card.toFloat();
        g.setColour (st.panel);
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (st.accent.withAlpha (0.7f));
        g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.3f);

        auto inner = card.reduced (28, 24);
        inner.removeFromBottom (48);
        auto heart = inner.removeFromTop (40).removeFromLeft (40).toFloat();
        drawHeart (g, heart.reduced (6));
        g.setColour (st.text);
        g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
        g.drawFittedText (t ("Grazie per aver scaricato ", "Thank you for downloading ") + name,
                          card.reduced (28, 24).removeFromTop (40).withTrimmedLeft (50).withTrimmedRight (50),
                          juce::Justification::centredLeft, 2);

        inner.removeFromTop (12);
        juce::AttributedString s;
        s.setWordWrap (juce::AttributedString::byWord);
        s.setLineSpacing (5.0f);
        const auto bodyFont = juce::Font (juce::FontOptions (15.0f));
        s.append (t ("Ho voluto creare questo plug-in per condividere la passione per la musica suonata.\n\n"
                     "Se hai voglia di contribuire fai un'offerta su PayPal a ",
                     "I created this plug-in to share my passion for played music.\n\n"
                     "If you'd like to contribute, you can make a donation on PayPal to "), bodyFont, st.text.withAlpha (0.9f));
        s.append (kPayPalEmail, juce::Font (juce::FontOptions (15.0f, juce::Font::bold)), st.accent);
        s.append ("\n\n", bodyFont, st.text);
        s.append (t ("Grazie di cuore, e buona musica!\nAlfredo (Vince May), May Recording Studio",
                     "Heartfelt thanks, and enjoy the music!\nAlfredo (Vince May), May Recording Studio"),
                  juce::Font (juce::FontOptions (13.5f, juce::Font::italic)), st.dim);
        juce::TextLayout tl;
        tl.createLayout (s, (float) inner.getWidth());
        tl.draw (g, inner.toFloat());
    }

    void mouseDown (const juce::MouseEvent&) override {}

private:
    bool en() const { return english ? english() : false; }
    juce::String t (const char* it, const char* enText) const { return juce::String::fromUTF8 (en() ? enText : it); }

    void drawHeart (juce::Graphics& g, juce::Rectangle<float> r) const
    {
        juce::Path p;
        const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
        p.startNewSubPath (x + w * 0.5f, y + h);
        p.cubicTo (x - w * 0.15f, y + h * 0.45f, x + w * 0.15f, y - h * 0.15f, x + w * 0.5f, y + h * 0.25f);
        p.cubicTo (x + w * 0.85f, y - h * 0.15f, x + w * 1.15f, y + h * 0.45f, x + w * 0.5f, y + h);
        p.closeSubPath();
        g.setColour (st.accent);
        g.fillPath (p);
    }

    void close()
    {
        auto s = store();
        s->setValue ("welcomeSeen_" + key, true);
        s->saveIfNeeded();
        setVisible (false);
        if (onClose) onClose();
    }

    juce::String key, name;
    std::function<bool()> english;
    std::function<void (bool)> changeLanguage;
    WelcomeStyle st;
    WelcomeButtonLook buttonLook;
    juce::TextButton donateBtn, copyBtn, closeBtn, langBtn;
    juce::Rectangle<int> card;
    bool copied = false;
};

// Piccolo pulsante "Dona" / "Donate" da mettere in basso a destra
class DonateButton : public juce::Button
{
public:
    DonateButton (std::function<bool()> isEnglish, WelcomeStyle s = {})
        : juce::Button ("donate"), english (std::move (isEnglish)), st (s)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTooltip ("PayPal");
    }

    static constexpr int kWidth = 64, kHeight = 20;

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (st.accent.withAlpha (down ? 0.35f : (over ? 0.25f : 0.12f)));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (st.accent.withAlpha (over ? 0.95f : 0.7f));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText (juce::String::fromUTF8 ((english && english()) ? "Donate" : "Dona"), getLocalBounds(), juce::Justification::centred);
    }

private:
    std::function<bool()> english;
    WelcomeStyle st;
};
} // namespace may
