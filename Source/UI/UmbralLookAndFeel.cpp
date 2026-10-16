#include "UmbralLookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace umbral::ui
{
    juce::LookAndFeel_V4::ColourScheme UmbralLookAndFeel::createUmbralColourScheme() noexcept
    {
        return {
            tokens::umbra,    // windowBackground
            tokens::raised,   // widgetBackground
            tokens::panel,    // menuBackground
            tokens::hairline, // outline
            tokens::bone,     // defaultText
            tokens::raised,   // defaultFill
            tokens::umbra,    // highlightedText
            tokens::signal,   // highlightedFill
            tokens::bone      // menuText
        };
    }

    UmbralLookAndFeel::UmbralLookAndFeel()
        : juce::LookAndFeel_V4 (createUmbralColourScheme())
    {
        // Ventana y contenedores
        setColour (juce::ResizableWindow::backgroundColourId, tokens::umbra);
        setColour (juce::DocumentWindow::backgroundColourId,  tokens::umbra);

        // Botones y controles segmentados
        setColour (juce::TextButton::buttonColourId,    tokens::raised);
        setColour (juce::TextButton::buttonOnColourId,  tokens::signal);
        setColour (juce::TextButton::textColourOffId,   tokens::bone);
        setColour (juce::TextButton::textColourOnId,    tokens::umbra);

        // Sliders / Knobs
        setColour (juce::Slider::rotarySliderFillColourId,    tokens::raised);
        setColour (juce::Slider::rotarySliderOutlineColourId, tokens::hairline);
        setColour (juce::Slider::thumbColourId,               tokens::bone);
        setColour (juce::Slider::trackColourId,               tokens::signal);
        setColour (juce::Slider::backgroundColourId,          tokens::raised);
        setColour (juce::Slider::textBoxTextColourId,         tokens::bone);
        setColour (juce::Slider::textBoxBackgroundColourId,   tokens::raised);
        setColour (juce::Slider::textBoxOutlineColourId,      tokens::hairline);
        setColour (juce::Slider::textBoxHighlightColourId,    tokens::signal);

        // ComboBox y PopupMenu
        setColour (juce::ComboBox::backgroundColourId,     tokens::raised);
        setColour (juce::ComboBox::outlineColourId,        tokens::hairline);
        setColour (juce::ComboBox::textColourId,           tokens::bone);
        setColour (juce::ComboBox::arrowColourId,          tokens::muted);
        setColour (juce::ComboBox::focusedOutlineColourId, tokens::signal);

        setColour (juce::PopupMenu::backgroundColourId,            tokens::panel);
        setColour (juce::PopupMenu::textColourId,                  tokens::bone);
        setColour (juce::PopupMenu::headerTextColourId,            tokens::muted);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, tokens::raised);
        setColour (juce::PopupMenu::highlightedTextColourId,       tokens::signal);

        // Etiquetas y campos de texto
        setColour (juce::Label::textColourId,             tokens::bone);
        setColour (juce::Label::backgroundColourId,       juce::Colours::transparentBlack);
        setColour (juce::Label::outlineColourId,          juce::Colours::transparentBlack);
        setColour (juce::Label::textWhenEditingColourId,  tokens::bone);
        setColour (juce::Label::backgroundWhenEditingColourId, tokens::raised);
        setColour (juce::Label::outlineWhenEditingColourId,    tokens::signal);

        setColour (juce::TextEditor::backgroundColourId,     tokens::raised);
        setColour (juce::TextEditor::textColourId,           tokens::bone);
        setColour (juce::TextEditor::outlineColourId,        tokens::hairline);
        setColour (juce::TextEditor::focusedOutlineColourId, tokens::signal);
        setColour (juce::TextEditor::highlightColourId,      tokens::signal);
        setColour (juce::TextEditor::highlightedTextColourId, tokens::umbra);
    }

    void UmbralLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                              int x, int y, int width, int height,
                                              float sliderPosProportional,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (tokens::borderWidth);
        const float diameter = std::min (bounds.getWidth(), bounds.getHeight());
        if (diameter <= 2.0f)
            return;

        const float radius  = diameter * 0.5f;
        const auto centre   = bounds.getCentre();
        const auto bodyArea = juce::Rectangle<float> (diameter, diameter).withCentre (centre);

        const float alpha = slider.isEnabled() ? 1.0f : tokens::disabledOpacity;
        const float clampedPos = std::clamp (sliderPosProportional, 0.0f, 1.0f);
        const float angle = rotaryStartAngle + clampedPos * (rotaryEndAngle - rotaryStartAngle);

        // Cuerpo circular plano en color Raised + borde de 1 px Hairline (sin sombras ni arcos recargados)
        const auto fillColour    = slider.findColour (juce::Slider::rotarySliderFillColourId).withMultipliedAlpha (alpha);
        const auto outlineColour = slider.findColour (juce::Slider::rotarySliderOutlineColourId).withMultipliedAlpha (alpha);
        const auto needleColour  = slider.findColour (juce::Slider::thumbColourId).withMultipliedAlpha (alpha);

        g.setColour (fillColour);
        g.fillEllipse (bodyArea);

        g.setColour (outlineColour);
        g.drawEllipse (bodyArea, tokens::borderWidth);

        // Aguja indicadora radial en color Bone (1.5 px) desde el centro hasta el borde interior
        const float innerRadius = radius * 0.22f;
        const float outerRadius = std::max (innerRadius + 1.0f, radius - (tokens::borderWidth * 2.5f));

        const float sinA = std::sin (angle);
        const float cosA = -std::cos (angle);

        juce::Path needlePath;
        needlePath.startNewSubPath (centre.x + innerRadius * sinA, centre.y + innerRadius * cosA);
        needlePath.lineTo (centre.x + outerRadius * sinA, centre.y + outerRadius * cosA);

        g.setColour (needleColour);
        g.strokePath (needlePath, juce::PathStrokeType (tokens::knobNeedleStroke,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
    }

    void UmbralLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                                  juce::Button& button,
                                                  const juce::Colour& /*backgroundColour*/,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced (tokens::borderWidth * 0.5f);
        if (bounds.isEmpty())
            return;

        const float alpha = button.isEnabled() ? 1.0f : tokens::disabledOpacity;
        const bool isToggled = button.getToggleState();
        const bool outlineActiveStyle = static_cast<bool> (button.getProperties().getWithDefault (kOutlineActiveStyleProperty, false));

        if (isToggled && ! outlineActiveStyle)
        {
            // Estado activo estándar (RATE, Stereo/Mono): fondo sólido Signal, sin borde adicional
            const auto activeFill = button.findColour (juce::TextButton::buttonOnColourId).withMultipliedAlpha (alpha);
            g.setColour (activeFill);
            g.fillRoundedRectangle (bounds, tokens::radius);
        }
        else
        {
            // Estado inactivo o activo por borde (SYNC Locked to DAW): fondo Raised
            const auto inactiveFill = button.findColour (juce::TextButton::buttonColourId).withMultipliedAlpha (alpha);
            g.setColour (inactiveFill);
            g.fillRoundedRectangle (bounds, tokens::radius);

            juce::Colour borderColour = tokens::hairline;
            if (isToggled && outlineActiveStyle)
                borderColour = tokens::signal;
            else if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
                borderColour = tokens::hoverBorder();

            g.setColour (borderColour.withMultipliedAlpha (alpha));
            g.drawRoundedRectangle (bounds, tokens::radius, tokens::borderWidth);
        }

        if (button.hasKeyboardFocus (true))
        {
            g.setColour (tokens::signal.withMultipliedAlpha (alpha));
            g.drawRoundedRectangle (bounds, tokens::radius, tokens::borderWidth);
        }
    }

    void UmbralLookAndFeel::drawButtonText (juce::Graphics& g,
                                            juce::TextButton& button,
                                            bool /*shouldDrawButtonAsHighlighted*/,
                                            bool /*shouldDrawButtonAsDown*/)
    {
        const auto font = getTextButtonFont (button, button.getHeight());
        g.setFont (font);

        const float alpha = button.isEnabled() ? 1.0f : tokens::disabledOpacity;
        const bool isToggled = button.getToggleState();
        const bool outlineActiveStyle = static_cast<bool> (button.getProperties().getWithDefault (kOutlineActiveStyleProperty, false));

        juce::Colour textColour = button.findColour (juce::TextButton::textColourOffId);
        if (! button.isEnabled())
        {
            textColour = tokens::muted;
        }
        else if (isToggled)
        {
            textColour = outlineActiveStyle ? tokens::signal
                                            : button.findColour (juce::TextButton::textColourOnId);
        }

        g.setColour (textColour.withMultipliedAlpha (alpha));

        const int yIndent = std::min (4, button.proportionOfHeight (0.15f));
        const int xIndent = std::min (6, button.proportionOfWidth (0.10f));
        const auto textBounds = button.getLocalBounds().reduced (xIndent, yIndent);

        if (! textBounds.isEmpty())
            g.drawFittedText (button.getButtonText(), textBounds, juce::Justification::centred, 1);
    }

    void UmbralLookAndFeel::drawComboBox (juce::Graphics& g,
                                          int width, int height,
                                          bool isButtonDown,
                                          int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                          juce::ComboBox& box)
    {
        const auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (tokens::borderWidth * 0.5f);
        if (bounds.isEmpty())
            return;

        const float alpha = box.isEnabled() ? 1.0f : tokens::disabledOpacity;

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId).withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (bounds, tokens::radius);

        const auto borderColour = (isButtonDown || box.hasKeyboardFocus (true))
                                      ? tokens::hoverBorder()
                                      : box.findColour (juce::ComboBox::outlineColourId);
        g.setColour (borderColour.withMultipliedAlpha (alpha));
        g.drawRoundedRectangle (bounds, tokens::radius, tokens::borderWidth);

        // Chevron hacia abajo (v) en Muted
        const float arrowZoneWidth = 24.0f;
        const auto arrowZone = juce::Rectangle<float> (static_cast<float> (width) - arrowZoneWidth,
                                                       0.0f,
                                                       arrowZoneWidth,
                                                       static_cast<float> (height));
        const auto arrowCentre = arrowZone.getCentre();
        juce::Path chevron;
        chevron.startNewSubPath (arrowCentre.x - 4.0f, arrowCentre.y - 2.0f);
        chevron.lineTo (arrowCentre.x, arrowCentre.y + 2.5f);
        chevron.lineTo (arrowCentre.x + 4.0f, arrowCentre.y - 2.0f);

        g.setColour (box.findColour (juce::ComboBox::arrowColourId).withMultipliedAlpha (alpha));
        g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font UmbralLookAndFeel::getTextButtonFont (juce::TextButton& /*button*/, int /*buttonHeight*/)
    {
        return getModeButtonFont();
    }

    juce::Font UmbralLookAndFeel::getComboBoxFont (juce::ComboBox& /*box*/)
    {
        return getCurveNameFont();
    }

    juce::Font UmbralLookAndFeel::getLabelFont (juce::Label& label)
    {
        // Si la etiqueta ya tiene una altura personalizada válida, la preserva sobre la fuente fallback
        const auto current = label.getFont();
        const float height = current.getHeight() > 0.0f ? current.getHeight() : tokens::midValueSize;
        return makeSansFont (height, current.getStyleFlags());
    }

    juce::Font UmbralLookAndFeel::getPopupMenuFont()
    {
        return getCurveNameFont();
    }

    juce::Font UmbralLookAndFeel::getSliderPopupFont (juce::Slider& /*slider*/)
    {
        return getMidValueFont();
    }

    // --- Helpers de Jerarquía Tipográfica Fallback (DESIGN.md §4) ---

    juce::Font UmbralLookAndFeel::makeSansFont (float heightPx, int styleFlags, float tracking)
    {
        const float safeHeight = std::max (1.0f, heightPx);
        auto options = juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), safeHeight, styleFlags);
        if (tracking != 0.0f)
            options = options.withKerningFactor (tracking);
        return juce::Font (options);
    }

    juce::Font UmbralLookAndFeel::makeMonoFont (float heightPx, int styleFlags)
    {
        const float safeHeight = std::max (1.0f, heightPx);
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), safeHeight, styleFlags));
    }

    juce::Font UmbralLookAndFeel::getBrandFont (bool extraBold)
    {
        return makeSansFont (tokens::brandSize, extraBold ? juce::Font::bold : juce::Font::plain, 0.04f);
    }

    juce::Font UmbralLookAndFeel::getSectionHeaderFont()
    {
        return makeSansFont (tokens::sectionHeaderSize, juce::Font::bold, 0.12f);
    }

    juce::Font UmbralLookAndFeel::getCurveNameFont()
    {
        return makeSansFont (tokens::curveNameSize, juce::Font::bold);
    }

    juce::Font UmbralLookAndFeel::getModeButtonFont()
    {
        return makeSansFont (tokens::modeButtonSize, juce::Font::bold);
    }

    juce::Font UmbralLookAndFeel::getMeterLabelFont()
    {
        return makeSansFont (tokens::meterLabelSize, juce::Font::bold);
    }

    juce::Font UmbralLookAndFeel::getMixReadoutFont()
    {
        return makeMonoFont (tokens::mixReadoutSize, juce::Font::bold);
    }

    juce::Font UmbralLookAndFeel::getBpmValueFont()
    {
        return makeMonoFont (tokens::bpmValueSize, juce::Font::plain);
    }

    juce::Font UmbralLookAndFeel::getGainReadoutFont()
    {
        return makeMonoFont (tokens::gainReadoutSize, juce::Font::bold);
    }

    juce::Font UmbralLookAndFeel::getMidValueFont()
    {
        return makeMonoFont (tokens::midValueSize, juce::Font::plain);
    }

    juce::Font UmbralLookAndFeel::getBarPositionFont()
    {
        return makeMonoFont (tokens::barPositionSize, juce::Font::plain);
    }

    juce::Font UmbralLookAndFeel::getAxisScaleFont()
    {
        return makeMonoFont (tokens::axisScaleSize, juce::Font::plain);
    }
} // namespace umbral::ui
