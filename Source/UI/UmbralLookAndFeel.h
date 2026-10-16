#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DesignTokens.h"

namespace umbral::ui
{
    // LookAndFeel oficial de UMBRAL Duck (T-09).
    // Implementa la paleta oscura de alto contraste, controles planos sin sombras ni degradados
    // y la jerarquía tipográfica fallback de docs/DESIGN.md.
    class UmbralLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        // Propiedad opcional en juce::Button::getProperties() para botones cuyo estado activo
        // usa fondo Raised + borde y texto Signal (ej. SYNC Locked to DAW en DESIGN.md §6).
        static inline const juce::Identifier kOutlineActiveStyleProperty { "outlineActiveStyle" };

        UmbralLookAndFeel();
        ~UmbralLookAndFeel() override = default;

        // --- Sliders / Perillas Giratorias (DESIGN.md §5 y §6) ---
        void drawRotarySlider (juce::Graphics& g,
                               int x, int y, int width, int height,
                               float sliderPosProportional,
                               float rotaryStartAngle,
                               float rotaryEndAngle,
                               juce::Slider& slider) override;

        // --- Botones y Controles Segmentados (DESIGN.md §6) ---
        void drawButtonBackground (juce::Graphics& g,
                                   juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        void drawButtonText (juce::Graphics& g,
                             juce::TextButton& button,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;

        // --- Desplegables (CURVE Dropdown, DESIGN.md §6) ---
        void drawComboBox (juce::Graphics& g,
                           int width, int height,
                           bool isButtonDown,
                           int buttonX, int buttonY, int buttonW, int buttonH,
                           juce::ComboBox& box) override;

        // --- Overrides de Fuentes JUCE ---
        juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override;
        juce::Font getComboBoxFont (juce::ComboBox& box) override;
        juce::Font getLabelFont (juce::Label& label) override;
        juce::Font getPopupMenuFont() override;
        juce::Font getSliderPopupFont (juce::Slider& slider) override;

        // --- Jerarquía Tipográfica Fallback (DESIGN.md §4) ---
        // A. Familia Sans-Serif (fallback del sistema para Archivo)
        static juce::Font getBrandFont (bool extraBold = true);
        static juce::Font getSectionHeaderFont();
        static juce::Font getCurveNameFont();
        static juce::Font getModeButtonFont();
        static juce::Font getMeterLabelFont();

        // B. Familia Monoespaciada (fallback del sistema para IBM Plex Mono)
        static juce::Font getMixReadoutFont();
        static juce::Font getBpmValueFont();
        static juce::Font getGainReadoutFont();
        static juce::Font getMidValueFont();
        static juce::Font getBarPositionFont();
        static juce::Font getAxisScaleFont();

        // Constructores base de fuente fallback
        static juce::Font makeSansFont (float heightPx, int styleFlags = juce::Font::plain, float tracking = 0.0f);
        static juce::Font makeMonoFont (float heightPx, int styleFlags = juce::Font::plain);

    private:
        static ColourScheme createUmbralColourScheme() noexcept;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmbralLookAndFeel)
    };
} // namespace umbral::ui

namespace umbral
{
    using UmbralLookAndFeel = ui::UmbralLookAndFeel;
} // namespace umbral
