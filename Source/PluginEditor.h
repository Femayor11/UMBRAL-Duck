#pragma once

#include "PluginProcessor.h"
#include "UI/ControlStrip.h"
#include "UI/CurveBankGrid.h"
#include "UI/GainResponseView.h"
#include "UI/HeaderBar.h"
#include "UI/LevelMetersComponent.h"
#include "UI/UmbralLookAndFeel.h"

// Interfaz completa de UMBRAL Duck en tamano unico fijo de 780 x 540 px.
class UmbralDuckEditor final : public juce::AudioProcessorEditor,
                               private juce::ValueTree::Listener
{
public:
    explicit UmbralDuckEditor (UmbralDuckProcessor&);
    ~UmbralDuckEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    [[nodiscard]] umbral::ui::HeaderBar& getHeaderBar() noexcept { return headerBar; }
    [[nodiscard]] const umbral::ui::HeaderBar& getHeaderBar() const noexcept { return headerBar; }
    [[nodiscard]] umbral::ui::ControlStrip& getControlStrip() noexcept { return controlStrip; }
    [[nodiscard]] const umbral::ui::ControlStrip& getControlStrip() const noexcept { return controlStrip; }
    [[nodiscard]] umbral::ui::ChannelsSelector& getChannelsSelector() noexcept { return channelsSelector; }
    [[nodiscard]] const umbral::ui::ChannelsSelector& getChannelsSelector() const noexcept { return channelsSelector; }
    [[nodiscard]] umbral::ui::GainResponseView& getGainResponseView() noexcept { return gainResponseView; }
    [[nodiscard]] const umbral::ui::GainResponseView& getGainResponseView() const noexcept { return gainResponseView; }
    [[nodiscard]] umbral::ui::LevelMetersComponent& getLevelMeters() noexcept { return levelMeters; }
    [[nodiscard]] const umbral::ui::LevelMetersComponent& getLevelMeters() const noexcept { return levelMeters; }
    [[nodiscard]] umbral::ui::CurveBankGrid& getCurveBankGrid() noexcept { return curveBankGrid; }
    [[nodiscard]] const umbral::ui::CurveBankGrid& getCurveBankGrid() const noexcept { return curveBankGrid; }

private:
    void valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged) override;

    UmbralDuckProcessor& processorRef;
    umbral::ui::UmbralLookAndFeel lookAndFeel;
    umbral::ui::HeaderBar headerBar;
    umbral::ui::ControlStrip controlStrip;
    umbral::ui::GainResponseView gainResponseView;
    umbral::ui::LevelMetersComponent levelMeters;
    umbral::ui::ChannelsSelector channelsSelector;
    umbral::ui::CurveBankGrid curveBankGrid;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmbralDuckEditor)
};
