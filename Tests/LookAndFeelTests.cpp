#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Common/DesignTokens.h"
#include "UI/DesignTokens.h"
#include "UI/UmbralLookAndFeel.h"

#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE ("DesignTokens match official hex palette and geometry", "[ui][tokens]")
{
    SECTION ("Official 7 colour tokens match exact ARGB hex values from DESIGN.md")
    {
        CHECK (umbral::tokens::umbra.getARGB()    == 0xFF131311u);
        CHECK (umbral::tokens::panel.getARGB()    == 0xFF1A1A17u);
        CHECK (umbral::tokens::raised.getARGB()   == 0xFF22221Eu);
        CHECK (umbral::tokens::hairline.getARGB() == 0xFF33322Cu);
        CHECK (umbral::tokens::muted.getARGB()    == 0xFF8C8A7Eu);
        CHECK (umbral::tokens::bone.getARGB()     == 0xFFECE8DCu);
        CHECK (umbral::tokens::signal.getARGB()   == 0xFFC9F04Bu);
    }

    SECTION ("Derived colours and opacities match specification")
    {
        CHECK_THAT (umbral::tokens::disabledOpacity, WithinAbs (0.40f, 1.0e-6f));
        CHECK_THAT (umbral::tokens::hoverBoneAlpha,  WithinAbs (0.35f, 1.0e-6f));
        CHECK_THAT (umbral::tokens::curveFillAlpha,  WithinAbs (0.15f, 1.0e-6f));

        const auto expectedHover = umbral::tokens::hairline.interpolatedWith (umbral::tokens::muted, 0.5f);
        CHECK (umbral::tokens::hoverBorder().getARGB() == expectedHover.getARGB());

        const auto fill = umbral::tokens::curveFill();
        CHECK (fill.getRed()   == umbral::tokens::signal.getRed());
        CHECK (fill.getGreen() == umbral::tokens::signal.getGreen());
        CHECK (fill.getBlue()  == umbral::tokens::signal.getBlue());
        CHECK_THAT (fill.getFloatAlpha(), WithinAbs (0.15f, 1.0e-2f));
    }

    SECTION ("Window dimensions, strokes and sweep match DESIGN.md and AGENTS.md")
    {
        CHECK (umbral::tokens::windowFullWidth     == 960);
        CHECK (umbral::tokens::windowFullHeight    == 600);
        CHECK (umbral::tokens::windowCompactWidth  == 560);
        CHECK (umbral::tokens::windowCompactHeight == 368);

        CHECK_THAT (umbral::tokens::radius,           WithinAbs (4.0f, 1.0e-6f));
        CHECK_THAT (umbral::tokens::borderWidth,      WithinAbs (1.0f, 1.0e-6f));
        CHECK_THAT (umbral::tokens::knobSweepDegrees, WithinAbs (270.0f, 1.0e-6f));
        CHECK_THAT (umbral::tokens::curveStroke,      WithinAbs (2.5f, 1.0e-6f));
        CHECK_THAT (umbral::tokens::knobNeedleStroke, WithinAbs (1.5f, 1.0e-6f));
        CHECK_THAT (umbral::tokens::playheadWidth,    WithinAbs (1.0f, 1.0e-6f));
    }
}

TEST_CASE ("UmbralLookAndFeel instantiation and colour table registration", "[ui][lookandfeel]")
{
    umbral::ui::UmbralLookAndFeel laf;

    SECTION ("Window and container colours")
    {
        CHECK (laf.findColour (juce::ResizableWindow::backgroundColourId) == umbral::tokens::umbra);
        CHECK (laf.findColour (juce::DocumentWindow::backgroundColourId)  == umbral::tokens::umbra);
        CHECK (laf.findColour (juce::PopupMenu::backgroundColourId)       == umbral::tokens::panel);
    }

    SECTION ("TextButton and segmented control colours")
    {
        CHECK (laf.findColour (juce::TextButton::buttonColourId)   == umbral::tokens::raised);
        CHECK (laf.findColour (juce::TextButton::buttonOnColourId) == umbral::tokens::signal);
        CHECK (laf.findColour (juce::TextButton::textColourOffId)  == umbral::tokens::bone);
        CHECK (laf.findColour (juce::TextButton::textColourOnId)   == umbral::tokens::umbra);
    }

    SECTION ("Rotary Slider, ComboBox and Label colours")
    {
        CHECK (laf.findColour (juce::Slider::rotarySliderFillColourId)    == umbral::tokens::raised);
        CHECK (laf.findColour (juce::Slider::rotarySliderOutlineColourId) == umbral::tokens::hairline);
        CHECK (laf.findColour (juce::Slider::thumbColourId)               == umbral::tokens::bone);
        CHECK (laf.findColour (juce::ComboBox::backgroundColourId)        == umbral::tokens::raised);
        CHECK (laf.findColour (juce::ComboBox::outlineColourId)           == umbral::tokens::hairline);
        CHECK (laf.findColour (juce::ComboBox::textColourId)              == umbral::tokens::bone);
        CHECK (laf.findColour (juce::ComboBox::arrowColourId)             == umbral::tokens::muted);
        CHECK (laf.findColour (juce::Label::textColourId)                 == umbral::tokens::bone);
    }
}

TEST_CASE ("UmbralLookAndFeel returns valid fallback fonts and hierarchy sizes", "[ui][lookandfeel]")
{
    umbral::ui::UmbralLookAndFeel laf;

    SECTION ("Static typography hierarchy helpers match DESIGN.md sizes")
    {
        const auto brandBold   = umbral::ui::UmbralLookAndFeel::getBrandFont (true);
        const auto brandLight  = umbral::ui::UmbralLookAndFeel::getBrandFont (false);
        const auto secHeader   = umbral::ui::UmbralLookAndFeel::getSectionHeaderFont();
        const auto curveName   = umbral::ui::UmbralLookAndFeel::getCurveNameFont();
        const auto modeBtn     = umbral::ui::UmbralLookAndFeel::getModeButtonFont();
        const auto meterLabel  = umbral::ui::UmbralLookAndFeel::getMeterLabelFont();
        const auto mixReadout  = umbral::ui::UmbralLookAndFeel::getMixReadoutFont();
        const auto bpmVal      = umbral::ui::UmbralLookAndFeel::getBpmValueFont();
        const auto gainReadout = umbral::ui::UmbralLookAndFeel::getGainReadoutFont();
        const auto midVal      = umbral::ui::UmbralLookAndFeel::getMidValueFont();
        const auto barPos      = umbral::ui::UmbralLookAndFeel::getBarPositionFont();
        const auto axisScale   = umbral::ui::UmbralLookAndFeel::getAxisScaleFont();

        CHECK_THAT (brandBold.getHeight(),   WithinAbs (umbral::tokens::brandSize, 1.0e-4f));
        CHECK_THAT (brandLight.getHeight(),  WithinAbs (umbral::tokens::brandSize, 1.0e-4f));
        CHECK (brandBold.isBold());
        CHECK_FALSE (brandLight.isBold());

        CHECK_THAT (secHeader.getHeight(),   WithinAbs (umbral::tokens::sectionHeaderSize, 1.0e-4f));
        CHECK_THAT (curveName.getHeight(),   WithinAbs (umbral::tokens::curveNameSize, 1.0e-4f));
        CHECK_THAT (modeBtn.getHeight(),     WithinAbs (umbral::tokens::modeButtonSize, 1.0e-4f));
        CHECK_THAT (meterLabel.getHeight(),  WithinAbs (umbral::tokens::meterLabelSize, 1.0e-4f));
        CHECK_THAT (mixReadout.getHeight(),  WithinAbs (umbral::tokens::mixReadoutSize, 1.0e-4f));
        CHECK_THAT (bpmVal.getHeight(),      WithinAbs (umbral::tokens::bpmValueSize, 1.0e-4f));
        CHECK_THAT (gainReadout.getHeight(), WithinAbs (umbral::tokens::gainReadoutSize, 1.0e-4f));
        CHECK_THAT (midVal.getHeight(),      WithinAbs (umbral::tokens::midValueSize, 1.0e-4f));
        CHECK_THAT (barPos.getHeight(),      WithinAbs (umbral::tokens::barPositionSize, 1.0e-4f));
        CHECK_THAT (axisScale.getHeight(),   WithinAbs (umbral::tokens::axisScaleSize, 1.0e-4f));
    }

    SECTION ("LookAndFeel virtual font overrides return positive valid heights")
    {
        juce::TextButton button ("1/4");
        juce::ComboBox combo ("Curve");
        juce::Label label ("Header", "GAIN RESPONSE");
        juce::Slider slider;

        const auto btnFont    = laf.getTextButtonFont (button, 32);
        const auto comboFont  = laf.getComboBoxFont (combo);
        const auto labelFont  = laf.getLabelFont (label);
        const auto popupFont  = laf.getPopupMenuFont();
        const auto sliderFont = laf.getSliderPopupFont (slider);

        CHECK (btnFont.getHeight()    > 0.0f);
        CHECK (comboFont.getHeight()  > 0.0f);
        CHECK (labelFont.getHeight()  > 0.0f);
        CHECK (popupFont.getHeight()  > 0.0f);
        CHECK (sliderFont.getHeight() > 0.0f);

        CHECK_THAT (btnFont.getHeight(),   WithinAbs (umbral::tokens::modeButtonSize, 1.0e-4f));
        CHECK_THAT (comboFont.getHeight(), WithinAbs (umbral::tokens::curveNameSize, 1.0e-4f));
    }
}

TEST_CASE ("UmbralLookAndFeel offscreen rendering of rotary slider and buttons", "[ui][lookandfeel]")
{
    umbral::ui::UmbralLookAndFeel laf;

    SECTION ("Rotary slider renders Raised circular body and Bone needle without crashes")
    {
        // Dimension impar (81x81) para que el centro geometrico (40.5, 40.5) coincida con el centro del pixel x = 40
        juce::Image canvas (juce::Image::ARGB, 81, 81, true, juce::SoftwareImageType {});

        juce::Slider knob (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        knob.setLookAndFeel (&laf);
        knob.setBounds (0, 0, 81, 81);

        const float sweepRad = juce::degreesToRadians (umbral::tokens::knobSweepDegrees);
        const float startRad = -sweepRad * 0.5f;
        const float endRad   =  sweepRad * 0.5f;

        {
            juce::Graphics g (canvas);
            g.fillAll (umbral::tokens::umbra);

            // En sliderPos = 0.5f el angulo es 0 rad (apunta hacia arriba: x = 40.5, y < 40.5)
            laf.drawRotarySlider (g, 0, 0, 81, 81, 0.5f, startRad, endRad, knob);
        }

        // El centro (40, 40) queda dentro del radio interior de la aguja y tiene color Raised
        const auto centerPixel = canvas.getPixelAt (40, 40);
        CHECK (centerPixel.getARGB() == umbral::tokens::raised.getARGB());

        // Sobre la vertical superior (40, 20) pasa el eje central de la aguja (1.5 px) en color Bone exacto
        const auto needlePixel = canvas.getPixelAt (40, 20);
        CHECK (needlePixel.getARGB() == umbral::tokens::bone.getARGB());

        knob.setLookAndFeel (nullptr);
    }

    SECTION ("TextButton renders Raised when inactive and Signal when toggled")
    {
        juce::TextButton button ("1/4");
        button.setLookAndFeel (&laf);
        button.setBounds (0, 0, 64, 32);

        // 1. Estado inactivo
        {
            juce::Image canvas (juce::Image::ARGB, 64, 32, true, juce::SoftwareImageType {});
            button.setToggleState (false, juce::dontSendNotification);

            {
                juce::Graphics g (canvas);
                laf.drawButtonBackground (g, button, umbral::tokens::raised, false, false);
            }
            CHECK (canvas.getPixelAt (32, 16).getARGB() == umbral::tokens::raised.getARGB());

            {
                juce::Graphics g (canvas);
                laf.drawButtonText (g, button, false, false);
            }
        }

        // 2. Estado activo relleno (RATE / Stereo / Mono): fondo Signal
        {
            juce::Image canvas (juce::Image::ARGB, 64, 32, true, juce::SoftwareImageType {});
            button.setToggleState (true, juce::dontSendNotification);

            {
                juce::Graphics g (canvas);
                laf.drawButtonBackground (g, button, umbral::tokens::signal, false, false);
            }
            CHECK (canvas.getPixelAt (32, 16).getARGB() == umbral::tokens::signal.getARGB());

            {
                juce::Graphics g (canvas);
                laf.drawButtonText (g, button, false, false);
            }
        }

        // 3. Estado activo por borde/texto (SYNC Locked to DAW): fondo Raised + borde Signal
        {
            juce::Image canvas (juce::Image::ARGB, 64, 32, true, juce::SoftwareImageType {});
            button.getProperties().set (umbral::ui::UmbralLookAndFeel::kOutlineActiveStyleProperty, true);
            button.setToggleState (true, juce::dontSendNotification);

            {
                juce::Graphics g (canvas);
                laf.drawButtonBackground (g, button, umbral::tokens::raised, false, false);
            }
            CHECK (canvas.getPixelAt (32, 16).getARGB() == umbral::tokens::raised.getARGB());

            {
                juce::Graphics g (canvas);
                laf.drawButtonText (g, button, false, false);
            }
        }

        button.setLookAndFeel (nullptr);
    }
}
