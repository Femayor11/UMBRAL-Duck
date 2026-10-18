#include "UI/CurveBankGrid.h"

#include "Common/MathUtils.h"
#include "Parameters/ParameterIDs.h"
#include "UI/DesignTokens.h"
#include "UI/UmbralLookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace umbral::ui
{
    CurveBankGrid::CurveBankGrid (juce::AudioProcessorValueTreeState& apvts)
        : sectionHeaderFont (UmbralLookAndFeel::getSectionHeaderFont()),
          headerTitleText ("CURVE BANK")
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
            updateCurveFromParameter (static_cast<float> (dsp::defaultCurveIndex));
        }

        resized();
    }

    CurveBankGrid::~CurveBankGrid()
    {
        curveAttachment.reset();
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
        const int index = std::clamp (
            juce::roundToInt (math::finiteOr (rawIndexValue, static_cast<float> (dsp::defaultCurveIndex))),
            0,
            kNumCells - 1);
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

    void CurveBankGrid::setCustomCurveData (const dsp::CustomCurveData& curve)
    {
        if (customCurve == curve)
            return;

        customCurve = curve;
        rebuildCellPaths();
        repaint();
    }

    void CurveBankGrid::rebuildCellPaths()
    {
        for (int i = 0; i < kNumCells; ++i)
        {
            auto& cell = cells[static_cast<size_t> (i)];
            cell.previewPath.clear();
            cell.isDashed = (i == dsp::customCurveIndex);

            const auto plot = cell.previewArea;
            if (plot.isEmpty())
                continue;

            std::span<const dsp::Node> nodes;
            if (i == dsp::customCurveIndex && customCurve.isValid())
                nodes = customCurve.getSpan();
            else
                nodes = dsp::getNodes (i);

            juce::Path basePath;
            float prevY = plot.getBottom();

            for (int s = 0; s <= kPreviewSamples; ++s)
            {
                const float phi   = static_cast<float> (s) / static_cast<float> (kPreviewSamples);
                const float value = dsp::evaluateNodes (nodes, phi, 0.5f);
                const float x     = plot.getX() + phi * plot.getWidth();
                const float y     = plot.getBottom() - value * plot.getHeight();

                if (s == 0)
                {
                    basePath.startNewSubPath (x, y);
                }
                else
                {
                    // Si hay un escalon vertical abrupto (ej. 11 Trance Gate en phi = 0.50),
                    // inserta el vertice previo en la misma abscisa para trazar un angulo recto limpio.
                    if (std::abs (y - prevY) > plot.getHeight() * 0.5f)
                        basePath.lineTo (x, prevY);
                    basePath.lineTo (x, y);
                }
                prevY = y;
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
        headerTitleArea = { 0.0f, 0.0f, bounds.getWidth(), headerH };

        const float cellsTop  = headerH;
        const float gridH     = std::max (0.0f, bounds.getHeight() - cellsTop);
        const float gap       = static_cast<float> (t::curveBankCellGap);
        const float totalGapW = gap * static_cast<float> (kNumCols - 1);
        const float totalGapH = gap * static_cast<float> (kNumRows - 1);
        const float availW    = std::max (0.0f, bounds.getWidth() - totalGapW);
        const float availH    = std::max (0.0f, gridH - totalGapH);
        const float cellW     = availW / static_cast<float> (kNumCols);
        const float cellH     = availH / static_cast<float> (kNumRows);
        const float cellPad   = static_cast<float> (t::curveBankCellPadding);

        for (int i = 0; i < kNumCells; ++i)
        {
            const int row   = i / kNumCols;
            const int col   = i % kNumCols;
            const float x   = static_cast<float> (col) * (cellW + gap);
            const float y   = cellsTop + static_cast<float> (row) * (cellH + gap);

            auto& cell       = cells[static_cast<size_t> (i)];
            cell.bounds      = { x, y, cellW, cellH };
            cell.previewArea = cell.bounds.reduced (cellPad);
        }

        rebuildCellPaths();
    }

    void CurveBankGrid::paint (juce::Graphics& g)
    {
        namespace t = umbral::tokens;

        // 1. Cabecera: "CURVE BANK" a la izquierda
        g.setFont (sectionHeaderFont);
        g.setColour (t::muted);
        g.drawText (headerTitleText, headerTitleArea, juce::Justification::centredLeft, false);

        // 2. Matriz 2x8 de 16 celdas interactivas (solo miniatura vectorial)
        for (int i = 0; i < kNumCells; ++i)
        {
            const auto& cell      = cells[static_cast<size_t> (i)];
            const bool isSelected = (i == selectedCurveIndex);
            const bool isHovered  = (i == hoveredCellIndex);

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

            // Miniatura vectorial de la curva (ranura 15 precalculada como dashed fill path)
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
        }
    }
} // namespace umbral::ui
