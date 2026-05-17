/*
  ==============================================================================

    ExpressionXmlParser.cpp
    See ExpressionXmlParser.h for the schema this parses.

  ==============================================================================
*/

#include "ExpressionXmlParser.h"

//==============================================================================
// Names the engine binds itself (WTGENERATOR.md section 4.3 reserved
// variables, plus the noise() function). A declared parameter may not
// shadow any of them. ExprTk-builtin names (sin, cos, ...) are caught
// later by the engine's compile() - listing them all here would be brittle.
static bool nameIsReserved (const juce::String& s)
{
    return s == "t" || s == "sample_rate" || s == "pi" || s == "e"
        || s == "noise";
}

//==============================================================================
ExpressionParseResult ExpressionXmlParser::fail (const juce::String& message)
{
    ExpressionParseResult r;
    r.succeeded = false;
    r.error     = message;
    return r;
}

bool ExpressionXmlParser::isReservedName (const juce::String& s)
{
    return nameIsReserved (s);
}

bool ExpressionXmlParser::isValidIdentifier (const juce::String& s)
{
    if (s.isEmpty())
        return false;

    const auto first = s[0];
    if (! (juce::CharacterFunctions::isLetter (first) || first == '_'))
        return false;

    for (int i = 1; i < s.length(); ++i)
    {
        const auto c = s[i];
        if (! (juce::CharacterFunctions::isLetterOrDigit (c) || c == '_'))
            return false;
    }

    return true;
}

juce::String ExpressionXmlParser::validateParameter (
    const ExpressionParameter& p,
    const std::vector<ExpressionParameter>& existing)
{
    if (p.name.isEmpty())
        return "A <Float> parameter is missing its name attribute.";

    if (! isValidIdentifier (p.name))
        return "Parameter name \"" + p.name + "\" is not a valid identifier "
               "(letters, digits and underscore; must not start with a digit).";

    if (isReservedName (p.name))
        return "Parameter name \"" + p.name + "\" is reserved - t, sample_rate, "
               "pi, e and noise are bound by the engine.";

    for (const auto& other : existing)
        if (other.name == p.name)
            return "Parameter \"" + p.name + "\" is declared more than once.";

    if (p.minValue >= p.maxValue)
        return "Parameter \"" + p.name + "\" has minVal >= maxVal "
               "(" + juce::String (p.minValue) + " >= "
               + juce::String (p.maxValue) + ").";

    return {};
}

juce::String ExpressionXmlParser::validatePhasorName (
    const juce::String& name,
    const std::vector<ExpressionParameter>& params,
    const std::vector<ExpressionPhasor>& phasors)
{
    if (name.isEmpty())
        return "A <Phasor> is missing its name attribute.";

    if (! isValidIdentifier (name))
        return "Phasor name \"" + name + "\" is not a valid identifier "
               "(letters, digits and underscore; must not start with a digit).";

    if (isReservedName (name))
        return "Phasor name \"" + name + "\" is reserved - t, sample_rate, "
               "pi, e and noise are bound by the engine.";

    for (const auto& p : params)
        if (p.name == name)
            return "Phasor name \"" + name + "\" collides with a declared "
                   "parameter of the same name.";

    for (const auto& other : phasors)
        if (other.name == name)
            return "Phasor \"" + name + "\" is declared more than once.";

    return {};
}

//==============================================================================
ExpressionParseResult ExpressionXmlParser::parseFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return fail ("File not found: " + file.getFullPathName());

    auto xml = juce::parseXML (file);
    if (xml == nullptr)
        return fail (file.getFileName() + " is not well-formed XML.");

    return parseXml (*xml);
}

ExpressionParseResult ExpressionXmlParser::parseString (const juce::String& xmlText)
{
    auto xml = juce::parseXML (xmlText);
    if (xml == nullptr)
        return fail ("Text is not well-formed XML.");

    return parseXml (*xml);
}

ExpressionParseResult ExpressionXmlParser::parseXml (const juce::XmlElement& root)
{
    if (! root.hasTagName ("ParameterSet"))
        return fail ("Root element is <" + root.getTagName()
                     + ">, expected <ParameterSet>.");

    // output-mode defaults to "wavetable" in the shared schema (section 8.2);
    // an expression definition must opt in explicitly.
    const auto outputMode = root.getStringAttribute ("output-mode", "wavetable");
    if (outputMode != "expression")
        return fail ("output-mode is \"" + outputMode + "\"; expression mode "
                     "requires output-mode=\"expression\". Wavetable and render "
                     "definitions load in a later version.");

    ExpressionDefinition def;
    def.analysisTag = root.getStringAttribute ("analysis");

    // ---- Pass 1: collect parameters, phasors and the expression -------------
    // Phasor `freq` references are resolved against the parameter set in
    // pass 2, so a <Phasor> may appear before the parameter it names.
    struct RawPhasor { juce::String name, freq; };
    std::vector<RawPhasor>  rawPhasors;
    const juce::XmlElement* exprElement = nullptr;

    for (auto* child : root.getChildIterator())
    {
        const auto tag = child->getTagName();

        if (tag == "Expression")
        {
            if (exprElement == nullptr)
                exprElement = child;
            continue;
        }

        if (tag == "Phasor")
        {
            rawPhasors.push_back ({ child->getStringAttribute ("name").trim(),
                                    child->getStringAttribute ("freq").trim() });
            continue;
        }

        if (tag == "Int" || tag == "Bool" || tag == "Choice")
            return fail ("Parameter \"" + child->getStringAttribute ("name")
                         + "\" is of type <" + tag + ">. v1 expression mode "
                         "supports <Float> parameters only; Int, Bool and "
                         "Choice arrive with the v3 script UI.");

        if (tag != "Float")
            return fail ("Unexpected element <" + tag + "> inside <ParameterSet>.");

        ExpressionParameter p;
        p.name         = child->getStringAttribute ("name").trim();
        p.minValue     = child->getDoubleAttribute ("minVal", 0.0);
        p.maxValue     = child->getDoubleAttribute ("maxVal", 1.0);
        p.defaultValue = child->getDoubleAttribute ("defaultVal", p.minValue);

        if (auto reason = validateParameter (p, def.parameters); reason.isNotEmpty())
            return fail (reason);

        // A default outside its own range is a typo, not a fatal error -
        // clamp it rather than reject the whole signal definition.
        p.defaultValue = juce::jlimit (p.minValue, p.maxValue, p.defaultValue);

        def.parameters.push_back (std::move (p));
    }

    if ((int) def.parameters.size() > kMaxExpressionParameters)
        return fail ("Definition declares "
                     + juce::String ((int) def.parameters.size())
                     + " parameters; the maximum is "
                     + juce::String (kMaxExpressionParameters) + ".");

    // ---- Pass 2: validate and resolve phasors -------------------------------
    for (const auto& rp : rawPhasors)
    {
        if (auto reason = validatePhasorName (rp.name, def.parameters, def.phasors);
            reason.isNotEmpty())
            return fail (reason);

        if (rp.freq.isEmpty())
            return fail ("Phasor \"" + rp.name + "\" has no freq attribute.");

        ExpressionPhasor phasor;
        phasor.name = rp.name;

        // An identifier (starts with a letter or underscore) names a
        // parameter; anything else is parsed as a numeric constant.
        const auto firstChar = rp.freq[0];
        if (juce::CharacterFunctions::isLetter (firstChar) || firstChar == '_')
        {
            bool found = false;
            for (const auto& param : def.parameters)
                if (param.name == rp.freq) { found = true; break; }

            if (! found)
                return fail ("Phasor \"" + rp.name + "\" is driven by \"" + rp.freq
                             + "\", which is not a declared parameter.");

            phasor.freqParam = rp.freq;
        }
        else
        {
            phasor.freqConstant = rp.freq.getDoubleValue();
        }

        def.phasors.push_back (std::move (phasor));
    }

    // ---- Expression ---------------------------------------------------------
    if (exprElement == nullptr)
        return fail ("No <Expression> element found in <ParameterSet>.");

    // getAllSubText decodes XML entities and CDATA, so an expression using
    // < > & is written either escaped or wrapped in <![CDATA[ ... ]]>.
    def.expression = exprElement->getAllSubText().trim();
    if (def.expression.isEmpty())
        return fail ("The <Expression> element is empty.");

    ExpressionParseResult result;
    result.succeeded  = true;
    result.definition = std::move (def);
    return result;
}
