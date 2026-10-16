#pragma once

#include "PluginProcessor.h"
#include "UI/ControlStrip.h"
#include "UI/CurveBankGrid.h"
#include "UI/GainResponseView.h"
#include "UI/HeaderBar.h"
#include "UI/LevelMetersComponent.h"
#include "UI/UmbralLookAndFeel.h"

// T-10 / T-11 / T-12 / T-13: interfaz completa de UMBRAL Duck con soporte de doble vista fija
// Full (960x600) y Compact (560x360), HeaderBar, ControlStrip, GainResponseView,
// ChannelsSelector + LevelMetersComponent y CurveBankGrid.
class UmbralDuckEditor final : public juce::AudioProcessorEditor,
                               private juce::ValueTree::Listener
{
public:
    explicit UmbralDuckEditor (UmbralDuckProcessor&);
    ~UmbralDuckEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setCompactView (bool compact);
    void toggleViewMode();
    [[nodiscard]] bool isCompactView() const noexcept { return compactView; }

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
    [[nodiscard]] juce::TextButton& getViewModeButton() noexcept { return headerBar.getViewModeButton(); }
    [[nodiscard]] const juce::TextButton& getViewModeButton() const noexcept { return headerBar.getViewModeButton(); }

private:
    void applyViewMode (bool compact, bool persistToProcessor);
    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;
    void valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged) override;

    UmbralDuckProcessor& processorRef;
    umbral::ui::UmbralLookAndFeel lookAndFeel;
    umbral::ui::HeaderBar headerBar;
    umbral::ui::ControlStrip controlStrip;
    umbral::ui::GainResponseView gainResponseView;
    umbral::ui::LevelMetersComponent levelMeters;
    umbral::ui::ChannelsSelector channelsSelector;
    umbral::ui::CurveBankGrid curveBankGrid;
    bool compactView = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmbralDuckEditor)
};
