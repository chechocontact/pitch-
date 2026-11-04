#pragma once

#include "Core/Pattern.h"
#include "Theme.h"

// Call-out panel for the selected strokes: their own LFO (depth, rate, shape, fade-in) and smooth Bezier curve mode.
class LfoPanel final : public juce::Component
{
public:
    LfoPanel (pitchpaint::Lfo initial, bool bezier, const juce::String& heading);

    // live = true while a slider is being dragged (the caller wraps the drag in one undo step).
    std::function<void (pitchpaint::Lfo, bool bezier, bool live)> onChange;
    std::function<void()> onGestureStart, onGestureEnd;

    static constexpr double suggestedDepthCents = 30.0;

    void resized() override;

private:
    pitchpaint::Lfo current() const;
    void notify (bool live);

    juce::Label heading;
    juce::Label depthLabel { {}, "Depth" }, rateLabel { {}, "Rate" }, fadeLabel { {}, "Fade-in" }, shapeLabel { {}, "Shape" };
    juce::Slider depth, rate, fade;
    juce::ComboBox shape;
    juce::ToggleButton bezierToggle { "Smooth Bezier curve" };
    juce::TextButton offButton { "Off" };
    bool dragging = false;
};
