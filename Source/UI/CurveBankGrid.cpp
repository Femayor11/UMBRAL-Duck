#include "UI/CurveBankGrid.h"

#include "Common/MathUtils.h"
#include "Parameters/ParameterIDs.h"
#include "UI/DesignTokens.h"
#include "UI/UmbralLookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace umbral::ui
{
    namespace
    {
        constexpr const char* kSlotNumbers[CurveBankGrid::kNumCells] = {
            "01", "02", "03", "04", "05", "06",
            "07", "08", "09", "10", "11", "12"
        };

        constexpr const char* kCurveNames[CurveBankGrid::kNumCells] = {
            "Classic", "Sharp", "Smooth", "Tight",
            "Reverse", "Rhythmic", "Deep", "Light",
            "Pump", "Flow", "Step", "Custom"
        };
    } // namespace

    CurveBankGrid::CurveBankGrid (juce::AudioProcessorValueTreeState& apvts)
        : sectionHeaderFont (UmbralLookAndFeel::getSectionHeaderFont()),
          customNoteFont (UmbralLookAndFeel::getBarPositionFont()),
          slotNumberFont (UmbralLookAndFeel::getAxisScaleFont()),
          curveNameFont (UmbralLookAndFeel::getCurveNameFont()),
          headerTitleText ("CURVE BANK"),
          customNoteText (juce::CharPointer_UTF8 ("12 \xE2\x80\x94 Custom: editor coming in a later version"))
    {
        setOpaque (false);

        for (int i = 0; i < kNumCells; ++i)
            cells[static_cast<size_t> (i)].isDashed = (i == dsp::customCurveIndex);

        if (auto* curveParam = apvts.getParameter (params::rawIDs::curve))
        {
            curveAttachment = std::make_unique<juce::ParameterAttachment> (
                *curveParam,
                [this] (float newValue) { updateCurveFromParameter (newValue); },
                apvts.undoManager);
            curveAttachment->sendInitialUpdate();
        }
        else
        {
            updateCurveFromParameter (2.0f);
        }

        resized();
    }

    CurveBankGrid::~CurveBankGrid()
    {
        curveAttachment.reset();
    }

    juce::String CurveBankGrid::getCellNumberText (int index)
    {
        const int clamped = std::clamp (index, 0, kNumCells - 1);
        return juce::String (kSlotNumbers[clamped]);
    }

    juce::String CurveBankGrid::getCellNameText (int index)
    {
        const int clamped = std::clamp (index, 0, kNumCells - 1);
        return juce::String (kCurveNames[clamped]);
    }

    juce::Rectangle<float> CurveBankGrid::getCellBounds (int index) const noexcept
    {
        const int clamped = std::clamp (index, 0, kNumCells - 1);
        return cells[static_cast<size_t> (clamped)].bounds;
    }

    const juce::Path& CurveBankGrid::getCellPreviewPath (int index) const noexcept
    {
        const int clamped = std::clamp (index, 0, kNumCells - 1);
        return cells[static_cast<size_t> (clamped)].previewPath;
    }

    bool CurveBankGrid::isCellDashed (int index) const noexcept
    {
        const int clamped = std::clamp (index, 0, kNumCells - 1);
        return cells[static_cast<size_t> (clamped)].isDashed;
    }

    int CurveBankGrid::getCellIndexAtPosition (juce::Point<float> position) const noexcept
    {
        for (int i = 0; i < kNumCells; ++i)
        {
            if (cells[static_cast<size_t> (i)].bounds.contains (position))
                return i;
        }
        return -1;
    }

    void CurveBankGrid::selectCurveIndex (int index)
    {
        const int clamped = std::clamp (index, 0, kNumCells - 1);
        updateCurveFromParameter (static_cast<float> (clamped));

        if (curveAttachment != nullptr)
            curveAttachment->setValueAsCompleteGesture (static_cast<float> (clamped));
    }

    void CurveBankGrid::updateCurveFromParameter (float rawIndexValue)
    {
        const int index = std::clamp (juce::roundToInt (math::finiteOr (rawIndexValue, 2.0f)), 0, kNumCells - 1);
        if (selectedCurveIndex != index)
        {
            selectedCurveIndex = index;
            repaint();
        }
    }

    void CurveBankGrid::mouseMove (const juce::MouseEvent& event)
    {
        const int hitIndex = getCellIndexAtPosition (event.position);
        if (hoveredCellIndex != hitIndex)
        {
            hoveredCellIndex = hitIndex;
            repaint();
        }
    }

    void CurveBankGrid::mouseDown (const juce::MouseEvent& event)
    {
        const int hitIndex = getCellIndexAtPosition (event.position);
        if (hitIndex >= 0)
            selectCurveIndex (hitIndex);
    }

    void CurveBankGrid::mouseExit (const juce::MouseEvent& /*event*/)
    {
        if (hoveredCellIndex != -1)
        {
            hoveredCellIndex = -1;
            repaint();
        }
    }

    void CurveBankGrid::rebuildCellPaths()
    {
        for (int i = 0; i < kNumCells; ++i)
        {
            auto& cell = cells[static_cast<size_t> (i)];
            cell.previewPath.clear();
            cell.isDashed = (i == dsp::customCurveIndex);

            const auto plot = cell.previewArea.reduced (tokens::borderWidth * 2.0f, tokens::borderWidth * 3.0f);
            if (plot.isEmpty())
                continue;

            const auto nodes = dsp::getNodes (i);
            juce::Path basePath;

            for (int s = 0; s <= kPreviewSamples; ++s)
            {
                const float phi   = static_cast<float> (s) / static_cast<float> (kPreviewSamples);
                const float value = dsp::evaluateNodes (nodes, phi, 0.5f);
                const float x     = plot.getX() + phi * plot.getWidth();
                const float y     = plot.getBottom() - value * plot.getHeight();

                if (s == 0)
                    basePath.startNewSubPath (x, y);
                else
                    basePath.lineTo (x, y);
            }

            if (cell.isDashed)
            {
                constexpr float dashLengths[2] = { 3.5f, 3.0f };
                juce::PathStrokeType stroke (tokens::curveBankStrokeWidth,
                                             juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded);
                stroke.createDashedStroke (cell.previewPath, basePath, dashLengths, 2);
            }
            else
            {
                cell.previewPath = std::move (basePath);
            }
        }
    }

    void CurveBankGrid::resized()
    {
        namespace t = umbral::tokens;

        const auto bounds = getLocalBounds().toFloat();
        if (bounds.isEmpty())
            return;

        const float headerH = std::min (static_cast<float> (t::curveBankHeaderHeight), bounds.getHeight());
        headerTitleArea = { 0.0f, 0.0f, bounds.getWidth() * 0.35f, headerH };
        customNoteArea  = { bounds.getWidth() * 0.35f, 0.0f, bounds.getWidth() * 0.65f, headerH };

        const float cellsTop    = bounds.getHeight() - std::min (static_cast<float> (t::curveBankCellHeight),
                                                                 std::max (0.0f, bounds.getHeight() - headerH));
        const float cellH       = std::max (0.0f, bounds.getHeight() - cellsTop);
        const float gap         = static_cast<float> (t::curveBankCellGap);
        const float totalGapW   = gap * static_cast<float> (kNumCells - 1);
        const float availW      = std::max (0.0f, bounds.getWidth() - totalGapW);
        const float cellW       = availW / static_cast<float> (kNumCells);
        const float cellPad     = static_cast<float> (t::curveBankCellPadding);
        const float numberH     = static_cast<float> (t::curveBankNumberHeight);
        const float nameH       = static_cast<float> (t::curveBankNameHeight);

        for (int i = 0; i < kNumCells; ++i)
        {
            auto& cell = cells[static_cast<size_t> (i)];
            const float x = static_cast<float> (i) * (cellW + gap);
            cell.bounds = { x, cellsTop, cellW, cellH };

            auto inner = cell.bounds.reduced (cellPad);
            cell.nameArea    = inner.removeFromBottom (nameH);
            cell.numberArea  = inner.removeFromBottom (numberH);
            cell.previewArea = inner;
        }

        rebuildCellPaths();
    }

    void CurveBankGrid::paint (juce::Graphics& g)
    {
        namespace t = umbral::tokens;

        // 1. Cabecera: "CURVE BANK" a la izquierda y nota de la ranura 12 a la derecha
        g.setFont (sectionHeaderFont);
        g.setColour (t::muted);
        g.drawText (headerTitleText, headerTitleArea, juce::Justification::centredLeft, false);

        g.setFont (customNoteFont);
        g.setColour (t::muted);
        g.drawText (customNoteText, customNoteArea, juce::Justification::centredRight, false);

        // 2. Hilera de 12 celdas interactivas (01..12)
        for (int i = 0; i < kNumCells; ++i)
        {
            const auto& cell       = cells[static_cast<size_t> (i)];
            const bool isSelected  = (i == selectedCurveIndex);
            const bool isHovered   = (i == hoveredCellIndex);

            const auto cardBounds = cell.bounds.reduced (t::borderWidth * 0.5f);
            if (cardBounds.isEmpty())
                continue;

            // Fondo plano Raised
            g.setColour (t::raised);
            g.fillRoundedRectangle (cardBounds, t::radius);

            // Borde: Signal si esta activa, hoverBorder en hover, Hairline en reposo
            const juce::Colour borderColour = isSelected ? t::signal
                                                         : (isHovered ? t::hoverBorder() : t::hairline);
            g.setColour (borderColour);
            g.drawRoundedRectangle (cardBounds, t::radius, t::borderWidth);

            // Miniatura vectorial de la curva (ranura 12 ya precalculada como dashed fill path)
            const juce::Colour strokeColour = isSelected ? t::signal
                                                         : (isHovered ? t::bone : t::muted);
            g.setColour (strokeColour);
            if (cell.isDashed)
            {
                g.fillPath (cell.previewPath);
            }
            else
            {
                g.strokePath (cell.previewPath,
                              juce::PathStrokeType (t::curveBankStrokeWidth,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
            }

            // Numero de ranura 01..12 (IBM Plex Mono 10 px: Signal si activa, Muted si inactiva)
            g.setFont (slotNumberFont);
            g.setColour (isSelected ? t::signal : t::muted);
            g.drawText (kSlotNumbers[i], cell.numberArea, juce::Justification::centredLeft, false);

            // Nombre de la curva (Archivo SemiBold 13 px en Bone)
            g.setFont (curveNameFont);
            g.setColour (t::bone);
            g.drawText (kCurveNames[i], cell.nameArea, juce::Justification::centredLeft, false);
        }
    }
} // namespace umbral::ui
