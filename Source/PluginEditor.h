#pragma once

#include "PluginProcessor.h"
#include "UI/GainResponseView.h"
#include "UI/LevelMetersComponent.h"
#include "UI/UmbralLookAndFeel.h"

// T-10 / T-11: soporte de doble vista fija Full (960x600) y Compact (560x360),
// alternancia de modo y persistencia de isCompactView en el estado del plugin.
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

    [[nodiscard]] umbral::ui::GainResponseView& getGainResponseView() noexcept { return gainResponseView; }
    [[nodiscard]] const umbral::ui::GainResponseView& getGainResponseView() const noexcept { return gainResponseView; }
    [[nodiscard]] umbral::ui::LevelMetersComponent& getLevelMeters() noexcept { return levelMeters; }
    [[nodiscard]] const umbral::ui::LevelMetersComponent& getLevelMeters() const noexcept { return levelMeters; }
    [[nodiscard]] juce::TextButton& getViewModeButton() noexcept { return viewModeButton; }
    [[nodiscard]] const juce::TextButton& getViewModeButton() const noexcept { return viewModeButton; }

private:
    void applyViewMode (bool compact, bool persistToProcessor);
    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;
    void valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged) override;

    UmbralDuckProcessor& processorRef;
    umbral::ui::UmbralLookAndFeel lookAndFeel;
    umbral::ui::GainResponseView gainResponseView;
    umbral::ui::LevelMetersComponent levelMeters;
    juce::TextButton viewModeButton;
    bool compactView = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmbralDuckEditor)
};
