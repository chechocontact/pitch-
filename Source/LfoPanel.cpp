#include "LfoPanel.h"

using namespace theme;

LfoPanel::LfoPanel (pitchpaint::Lfo initial, bool bezier, const juce::String& headingText)
{
    heading.setText (headingText, juce::dontSendNotification);
    heading.setFont (juce::Font (type::emphasis()));
    addAndMakeVisible (heading);

    const auto setUp = [this] (juce::Slider& s, juce::Label& l, double min, double max, double step, const juce::String& suffix,
                               double value, const juce::String& title, const juce::String& tooltip)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 76, controlHeight - 4);
        s.setRange (min, max, step);
        s.setTextValueSuffix (suffix);
        s.setValue (value, juce::dontSendNotification);
        s.setTitle (title);
        s.setTooltip (tooltip);
        s.onDragStart = [this] { dragging = true; if (onGestureStart) onGestureStart(); };
        s.onValueChange = [this] { notify (dragging); };
        s.onDragEnd = [this] { dragging = false; if (onGestureEnd) onGestureEnd(); };
        addAndMakeVisible (s);
        l.setFont (juce::Font (type::caption()));
        l.setColour (juce::Label::textColourId, colour::textSecondary);
        l.attachToComponent (&s, true);
    };

    setUp (depth, depthLabel, 0.0, 400.0, 1.0, " ct", initial.depthCents, "LFO depth",
           "How far the LFO moves the pitch either way, in cents (100 = one semitone). 0 = LFO off.");
    setUp (rate, rateLabel, 0.1, 30.0, 0.1, " Hz", initial.rateHz, "LFO rate", "LFO speed in cycles per second.");
    setUp (fade, fadeLabel, 0.0, 8.0, 0.05, " beats", initial.fadeBeats, "LFO fade-in",
           "How long after the note starts the LFO takes to reach full depth.");

    shape.addItemList ({ "Sine", "Triangle", "Saw", "Square" }, 1);
    shape.setSelectedItemIndex ((int) initial.shape, juce::dontSendNotification);
    shape.setTitle ("LFO shape");
    shape.onChange = [this] { notify (false); };
    addAndMakeVisible (shape);
    shapeLabel.setFont (juce::Font (type::caption()));
    shapeLabel.setColour (juce::Label::textColourId, colour::textSecondary);
    shapeLabel.attachToComponent (&shape, true);

    bezierToggle.setToggleState (bezier, juce::dontSendNotification);
    bezierToggle.setTooltip ("Join the stroke's points with smooth Bezier curves instead of straight lines.");
    bezierToggle.onClick = [this] { notify (false); };
    addAndMakeVisible (bezierToggle);

    offButton.setTooltip ("Remove the LFO (depth 0).");
    offButton.onClick = [this] { depth.setValue (0.0, juce::sendNotificationSync); };
    addAndMakeVisible (offButton);

    setSize (360, 2 * controlHeight + 4 * (controlHeight + space::s) + 3 * space::s + 8);
}

pitchpaint::Lfo LfoPanel::current() const
{
    return { depth.getValue(), rate.getValue(), (pitchpaint::LfoShape) juce::jmax (0, shape.getSelectedItemIndex()), fade.getValue() };
}

void LfoPanel::notify (bool live)
{
    if (onChange)
        onChange (current(), bezierToggle.getToggleState(), live);
}

void LfoPanel::resized()
{
    auto area = getLocalBounds().reduced (space::s);
    auto top = area.removeFromTop (controlHeight);
    offButton.setBounds (top.removeFromRight (56));
    heading.setBounds (top);
    for (auto* c : std::initializer_list<juce::Component*> { &depth, &rate, &fade, &shape })
    {
        area.removeFromTop (space::s);
        c->setBounds (area.removeFromTop (controlHeight).withTrimmedLeft (64));
    }
    area.removeFromTop (space::s);
    bezierToggle.setBounds (area.removeFromTop (controlHeight));
}
