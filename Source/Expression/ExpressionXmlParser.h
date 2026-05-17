/*
  ==============================================================================

    ExpressionXmlParser.h
    Parses the self-contained XML form of an expression-mode signal
    definition (WTGENERATOR.md section 4.3):

      <ParameterSet analysis="GenericOverlay" output-mode="expression">
        <Float name="carrier_hz" minVal="20" maxVal="20000" defaultVal="1000"/>
        ...
        <Expression>sin(2*pi*carrier_hz*t)</Expression>
      </ParameterSet>

    Stateless - all entry points are static. The parser validates structure
    and parameter declarations only; the expression's math is validated when
    ExpressionEngine compiles it.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ExpressionDefinition.h"

//==============================================================================
// Outcome of a parse attempt. On failure `error` is a single human-readable
// sentence suitable for display in the editor (PRINCIPLES.md section 1 -
// honest, actionable failure rather than a silent empty result).
struct ExpressionParseResult
{
    bool                 succeeded = false;
    ExpressionDefinition definition;
    juce::String         error;
};

//==============================================================================
class ExpressionXmlParser
{
public:
    // Reads and parses a .xml file. Fails cleanly if the file is missing or
    // not well-formed XML.
    static ExpressionParseResult parseFile (const juce::File& file);

    // Parses XML held in a string (used by tests and the v3 script path,
    // where the XML may not come straight off disk).
    static ExpressionParseResult parseString (const juce::String& xmlText);

    // Validates an already-parsed XML tree against the expression-mode
    // schema.
    static ExpressionParseResult parseXml (const juce::XmlElement& root);

private:
    static ExpressionParseResult fail (const juce::String& message);

    // Returns an empty string when `p` is valid, or a human-readable reason
    // why it is not. `existing` is the set of parameters already accepted,
    // for duplicate-name detection.
    static juce::String validateParameter (const ExpressionParameter& p,
                                           const std::vector<ExpressionParameter>& existing);

    static bool isValidIdentifier (const juce::String& s);
    static bool isReservedName    (const juce::String& s);
};
