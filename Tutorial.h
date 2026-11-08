#pragma once
#include "Theme.h"

// Tutorial a schede mostrato alla prima apertura (riapribile con il pulsante "?")
namespace mpm::ui
{
class Tutorial : public juce::Component
{
public:
    std::function<void()> onClose;

    static std::unique_ptr<juce::PropertiesFile> settings() { return SharedBus::settings(); }

    static bool alreadySeen()
    {
        auto s = settings();
        return s->getBoolValue ("tutorialSeen", false);
    }

    static void markSeen()
    {
        auto s = settings();
        s->setValue ("tutorialSeen", true);
        s->saveIfNeeded();
    }

    Tutorial()
    {
        for (auto* b : { &backBtn, &nextBtn, &skipBtn })
            addAndMakeVisible (b);
        backBtn.onClick = [this] { step = juce::jmax (0, step - 1); update(); };
        nextBtn.onClick = [this]
        {
            if (step < numSteps() - 1) { ++step; update(); }
            else close();
        };
        skipBtn.onClick = [this] { close(); };
        nextBtn.setToggleState (true, juce::dontSendNotification);
        setInterceptsMouseClicks (true, true);
        update();
    }

    void start() { step = 0; update(); setVisible (true); toFront (true); }

    void update()
    {
        backBtn.setButtonText (tr ("Indietro", "Back"));
        skipBtn.setButtonText (tr ("Salta", "Skip"));
        nextBtn.setButtonText (step == numSteps() - 1 ? tr ("Inizia a mixare", "Start mixing") : tr ("Avanti", "Next"));
        backBtn.setEnabled (step > 0);
        repaint();
    }

    void resized() override
    {
        card = getLocalBounds().withSizeKeepingCentre (juce::jmin (620, getWidth() - 40), 400);
        auto b = card.reduced (24).removeFromBottom (34);
        nextBtn.setBounds (b.removeFromRight (150));
        b.removeFromRight (8);
        backBtn.setBounds (b.removeFromRight (110));
        skipBtn.setBounds (b.removeFromLeft (90));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.62f));
        auto r = card.toFloat();
        g.setColour (C::panel);
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (C::teal2);
        g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.2f);

        auto inner = card.reduced (28, 24);
        auto head = inner.removeFromTop (54);
        auto badge = head.removeFromLeft (54).toFloat();
        g.setColour (C::teal);
        g.fillEllipse (badge.reduced (4));
        g.setColour (juce::Colour (0xff0f1a1c));
        g.setFont (font (22.0f, true));
        g.drawText (juce::String (step + 1), badge, juce::Justification::centred);

        head.removeFromLeft (14);
        g.setColour (C::dim);
        g.setFont (font (11.0f, true));
        g.drawText ((tr ("Guida rapida  |  passo ", "Quick guide  |  step ") + juce::String (step + 1) + " / " + juce::String (numSteps())).toUpperCase(),
                    head.removeFromTop (18), juce::Justification::bottomLeft);
        g.setColour (C::text);
        g.setFont (font (20.0f, true));
        g.drawText (title (step), head, juce::Justification::centredLeft);

        inner.removeFromTop (14);
        inner.removeFromBottom (50);
        juce::AttributedString s;
        s.setWordWrap (juce::AttributedString::byWord);
        s.setLineSpacing (4.0f);
        s.append (body (step), font (14.0f), juce::Colour (0xffc4ced8));
        juce::TextLayout tl;
        tl.createLayout (s, (float) inner.getWidth());
        tl.draw (g, inner.toFloat());

        // pallini di avanzamento
        const float dotsW = numSteps() * 16.0f;
        float x = card.getCentreX() - dotsW * 0.5f;
        for (int i = 0; i < numSteps(); ++i, x += 16.0f)
        {
            g.setColour (i == step ? C::teal : C::line);
            g.fillEllipse (x + 4, (float) card.getBottom() - 74.0f, 8, 8);
        }
    }

    void mouseDown (const juce::MouseEvent&) override {}

private:
    static int numSteps() { return 6; }

    static juce::String title (int i)
    {
        switch (i)
        {
            case 0: return tr ("Benvenuto in May Pro Mix", "Welcome to May Pro Mix");
            case 1: return tr ("Su ogni traccia: May Pro Mix Track", "On every track: May Pro Mix Track");
            case 2: return tr ("Sul canale Main: May Pro Mix Master", "On the Main channel: May Pro Mix Master");
            case 3: return tr ("Fai suonare il brano", "Play the song");
            case 4: return tr ("Leggi e applica i consigli", "Read and apply the advice");
            default: return tr ("Riferimento, lingua e aiuto", "Reference, language and help");
        }
    }

    static juce::String body (int i)
    {
        switch (i)
        {
            case 0: return tr ("May Pro Mix ascolta tutte le tracce del tuo brano e ti dice, traccia per traccia, cosa sistemare: livello, equalizzazione, compressione, spazio tra gli strumenti e loudness finale. Ti suggerisce anche quali plug-in usare, partendo da quelli di Fender Studio Pro e poi da quelli che hai installato.\n\nIl plug-in non tocca il suono al posto tuo: ti guida, e l'orecchio finale resta il tuo.",
                               "May Pro Mix listens to every track of your song and tells you, track by track, what to fix: level, EQ, compression, space between instruments and final loudness. It also suggests which plug-ins to use, starting with the Fender Studio Pro ones and then those you have installed.\n\nThe plug-in never changes the sound for you: it guides you, and your ears have the final say.");
            case 1: return tr ("Inserisci May Pro Mix Track come ULTIMO insert di ogni traccia (dopo EQ e compressori). Scegli il ruolo (voce, basso, cassa, chitarra...) oppure lascia Automatico: se le tracce si chiamano \"Voce\", \"Basso\", \"Kick\" il ruolo viene riconosciuto dal nome.\n\nSui bus di gruppo scegli \"Bus / gruppo\": cosi' non vengono contati due volte.",
                               "Insert May Pro Mix Track as the LAST insert on every track (after EQ and compressors). Choose the role (vocal, bass, kick, guitar...) or leave Automatic: if tracks are named \"Vocal\", \"Bass\", \"Kick\" the role is recognised from the name.\n\nOn group buses choose \"Bus / group\" so they are not counted twice.");
            case 2: return tr ("Inserisci May Pro Mix Master come ultimo plug-in sul canale Main. Qui scegli il GENERE del brano (rock, country rock, ballata, blues...) e l'obiettivo di loudness: i consigli su livelli, timbro e compressione cambiano in base al genere.",
                               "Insert May Pro Mix Master as the last plug-in on the Main channel. Here you choose the song GENRE (rock, country rock, ballad, blues...) and the loudness target: advice on levels, tone and compression changes with the genre.");
            case 3: return tr ("Premi Play e lascia suonare il brano, meglio se dall'inizio alla fine (servono almeno 20 secondi di musica). Il plug-in misura loudness, picchi, dinamica e spettro di ogni traccia.\n\nDopo una modifica importante (un EQ, un compressore) premi \"Azzera analisi\" e fai suonare di nuovo. Se metti in mute una traccia, spegni il suo \"Analizza\" (o metti il plug-in in bypass): Studio Pro non avvisa i plug-in del mute.",
                               "Press Play and let the song run, ideally from start to finish (at least 20 seconds of music are needed). The plug-in measures loudness, peaks, dynamics and spectrum of every track.\n\nAfter a major change (an EQ, a compressor) press \"Reset analysis\" and play again. If you mute a track, switch off its \"Analyse\" (or bypass the plug-in): Studio Pro does not tell plug-ins about mutes.");
            case 4: return tr ("Nel Master clicca una traccia per vedere i suoi consigli; la riga MIX mostra quelli generali. Rosso = importante, giallo = consigliato, verde = informazione.\n\n\"Applica correzione\" imposta da solo EQ, passa-alto, compressore o de-esser integrati, segna Fatto e rifa' l'analisi; \"Bilancia tutte\" nel Master sistema i livelli in un colpo solo. Se preferisci i plug-in nativi, trascina le etichette Pro EQ3 o Compressor sulla traccia.",
                               "In the Master click a track to see its advice; the MIX row shows the general advice. Red = important, yellow = suggested, green = information.\n\n\"Apply fix\" sets the built-in EQ, high-pass, compressor or de-esser, ticks Done and re-analyses; \"Balance all\" in the Master fixes every level at once. If you prefer native plug-ins, drag the Pro EQ3 or Compressor labels onto the track.");
            default: return tr ("Vuoi suonare come un disco che ami? Fai suonare quel brano sul Master, premi \"Memorizza riferimento\" e attiva \"Riferimento\": il bilanciamento tonale verra' confrontato con il suo.\n\nIT / EN cambia la lingua di tutte le istanze. Il pulsante \"?\" riapre questa guida; il manuale completo e' incluso nel pacchetto.",
                               "Want to sound like a record you love? Play that song through the Master, press \"Store reference\" and enable \"Reference\": the tonal balance will be compared with it.\n\nIT / EN switches the language of every instance. The \"?\" button reopens this guide; the full manual is included in the package.");
        }
    }

    void close()
    {
        markSeen();
        setVisible (false);
        if (onClose) onClose();
    }

    juce::TextButton backBtn, nextBtn, skipBtn;
    juce::Rectangle<int> card;
    int step = 0;
};
} // namespace mpm::ui
