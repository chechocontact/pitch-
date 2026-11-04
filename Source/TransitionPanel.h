#pragma once

#include "Core/Pattern.h"
#include "Theme.h"

// Call-out with the five fixed transition shapes. Each button draws its own curve, so the choice is visible.
class TransitionPanel final : public juce::Component
{
public:
    TransitionPanel (pitchpaint::Transition current, const juce::String& heading);
    ~TransitionPanel() override;

    std::function<void (pitchpaint::Transition)> onChoose;

    void resized() override;

private:
    class ShapeButton;
    juce::Label heading, hint;
    juce::OwnedArray<ShapeButton> buttons;
};
