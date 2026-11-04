#include "TransitionPanel.h"

using namespace theme;
using pitchpaint::Transition;

class TransitionPanel::ShapeButton final : public juce::Button
{
public:
    ShapeButton (Transition t, const juce::String& name, const juce::String& tip) : juce::Button (name), shape (t)
    {
        setTooltip (tip);
        setTitle (name);
        setClickingTogglesState (false);
        setWantsKeyboardFocus (true);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.5f);
        const bool on = getToggleState();
        g.setColour (on ? colour::accent.withAlpha (0.28f) : (over || down ? colour::textSecondary.withAlpha (0.14f) : colour::textSecondary.withAlpha (0.07f)));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? colour::accent : colour::textSecondary.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 6.0f, on ? 2.0f : 1.0f);

        // The curve: a note on the left rising to a note on the right, drawn as the shape would move.
        auto plot = r.reduced (10.0f, 9.0f);
        plot.removeFromBottom (14.0f);
        g.setColour (colour::textSecondary.withAlpha (0.25f));
        g.drawHorizontalLine ((int) plot.getBottom(), plot.getX(), plot.getRight());
        g.drawHorizontalLine ((int) plot.getY(), plot.getX(), plot.getRight());

        juce::Path path;
        for (int i = 0; i <= 40; ++i)
        {
            const double u = i / 40.0;
            const float x = plot.getX() + (float) u * plot.getWidth();
            const float y = plot.getBottom() - (float) pitchpaint::transitionShape (shape, u) * plot.getHeight();
            i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
        }
        g.setColour (on ? colour::accent : colour::textPrimary);
        g.strokePath (path, juce::PathStrokeType (2.2f));

        g.setFont (juce::Font (type::caption()));
        g.setColour (on ? colour::textPrimary : colour::textSecondary);
        g.drawText (getName(), getLocalBounds().removeFromBottom (18), juce::Justification::centred);
    }

private:
    Transition shape;
};

TransitionPanel::TransitionPanel (Transition current, const juce::String& headingText)
{
    heading.setText (headingText, juce::dontSendNotification);
    heading.setFont (juce::Font (type::emphasis()));
    addAndMakeVisible (heading);

    hint.setText ("How the pitch moves from one note to the next.", juce::dontSendNotification);
    hint.setFont (juce::Font (type::caption()));
    hint.setColour (juce::Label::textColourId, colour::textSecondary);
    addAndMakeVisible (hint);

    struct Entry { Transition t; const char* name; const char* tip; };
    static const Entry entries[] = {
        { Transition::fastExp, "Fast exp", "Leaves the first note almost at once, then settles gently into the next." },
        { Transition::fast,    "Fast",     "Leaves the first note quickly, between fast exponential and linear." },
        { Transition::linear,  "Linear",   "Moves at a constant speed." },
        { Transition::slow,    "Slow",     "Stays on the first note a while, between linear and slow exponential." },
        { Transition::slowExp, "Slow exp", "Stays on the first note, then rushes to the next at the end." },
    };

    for (const auto& e : entries)
    {
        auto* b = buttons.add (new ShapeButton (e.t, e.name, e.tip));
        b->setToggleState (e.t == current, juce::dontSendNotification);
        b->onClick = [this, b, t = e.t]
        {
            for (auto* other : buttons)
                other->setToggleState (other == b, juce::dontSendNotification);
            if (onChoose)
                onChoose (t);
        };
        addAndMakeVisible (b);
    }

    setSize (5 * 74 + 2 * space::s + 4 * 4, 2 * controlHeight + 78 + 3 * space::s);
}

TransitionPanel::~TransitionPanel() = default;

void TransitionPanel::resized()
{
    auto area = getLocalBounds().reduced (space::s);
    heading.setBounds (area.removeFromTop (controlHeight));
    hint.setBounds (area.removeFromTop (controlHeight - 6));
    area.removeFromTop (space::s);
    const int w = (area.getWidth() - 4 * 4) / 5;
    for (auto* b : buttons)
    {
        b->setBounds (area.removeFromLeft (w).withHeight (78));
        area.removeFromLeft (4);
    }
}
