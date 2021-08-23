/* -*- c++ -*- */

#include <sstream>
#include <iostream>
#include <algorithm>
#include "HttpAcceptParser.h"

std::string HttpAcceptParser::parse(const std::string & acceptValue, const std::vector<std::string> & availableContentTypes)
{
    // If the 'Accept' header is empty then return the first available content type.
    if (acceptValue.empty())
    {
        if (!availableContentTypes.empty())
        {
            return availableContentTypes.front();
        }
        return std::string();
    }

    std::istringstream acceptStream(acceptValue);
    std::vector<ParsedContentType> acceptedContentTypes;

    int order = 0;
    for (std::string token; std::getline(acceptStream, token, ','); ++order)
    {
        ParsedContentType contentType{std::move(token), "", "", 1.0f, order};
        bool contentTypeIsAccepted = true;
        std::istringstream tokenStream(trim(contentType.range));

        // Parse token parameters
        bool isFirstParameter = true;
        for (std::string param; std::getline(tokenStream, param, ';') && contentTypeIsAccepted;)
        {
            trim(param);
            if (isFirstParameter)
            {
                // Parse the media-range
                // ( "*/*" | ( type "/" "*" ) | ( type "/" subtype ) )
                stringToLower(param);
                contentType.range = std::move(param);
                const auto indexSlash = contentType.range.find('/');
                if (indexSlash == std::string::npos)
                {
                    // Invalid content type format.
                    contentTypeIsAccepted = false;
                    continue;
                }
                contentType.type = std::string(contentType.range.begin(), contentType.range.begin() + indexSlash);
                contentType.subtype = std::string(contentType.range.begin() + indexSlash + 1, contentType.range.end());
                if ((contentType.type == "*") && (contentType.subtype != "*"))
                {
                    // Invalid content type. Contains wildcard type with a subtype.
                    contentTypeIsAccepted = false;
                    continue;
                }
                isFirstParameter = false;
            }
            else
            {
                // Parse the Quality parameter if present
                // ";" ( "q" | "Q" ) "=" qvalue
                const auto indexEqual = param.find('=');
                if (indexEqual == std::string::npos)
                {
                    // Invalid syntax. A '=' token is expected, but no one is provided. Current content type should be discarded.
                    contentTypeIsAccepted = false;
                    continue;
                }
                auto key = std::string(param.begin(), param.begin() + indexEqual);
                trim(key);
                auto value = std::string(param.begin() + indexEqual + 1, param.end());
                trim(value);

                if ((key == "q") || (key == "Q"))
                {
                    if (!stringToFloat(value, &contentType.qvalue))
                    {
                        // Invalid quality value. A valid float value is expected. Current content type should be discarded.
                        contentTypeIsAccepted = false;
                        continue;
                    }

                    // RFC 7231 Section 5.3.1
                    if (((contentType.qvalue < 0.001f) && (contentType.qvalue != 0)) || (contentType.qvalue > 1.0f))
                    {
                        // Invalid value. Quality is normalized to a real number in the range 0 through 1,
                        // where 0.001 is the least preferred and 1 is the most preferred; A value of 0
                        // means "not acceptable".If no "q" parameter is present the default quality is 1.
                        contentType.qvalue = 1.0f;
                    }
                    else if (contentType.qvalue == 0)
                    {
                        // A value of 0 means "not acceptable".
                        contentType.qvalue = -1.0f;
                    }
                }
            }
        }

        if (contentTypeIsAccepted)
        {
            acceptedContentTypes.push_back(std::move(contentType));
        }
    }

    // Sort accepted content types by priority
    std::sort(acceptedContentTypes.begin(), acceptedContentTypes.end(), compareContentTypes);

    // Selects the most preferable content type from the available content types taking in consideration the accepted types.
    return getPreferableContentType(acceptedContentTypes, availableContentTypes);
}

bool HttpAcceptParser::stringToFloat(const std::string &s, float *f)
{
    try
    {
        *f = std::stof(s);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

inline std::string & HttpAcceptParser::rtrim(std::string &s, const char *t)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

inline std::string & HttpAcceptParser::ltrim(std::string &s, const char *t)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

std::string & HttpAcceptParser::trim(std::string &s)
{
    const char *charsToTrim = " \t\n\r\f\v";
    return ltrim(rtrim(s, charsToTrim), charsToTrim);
}

std::string & HttpAcceptParser::stringToLower(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c) { return std::tolower(c); });
    return s;
}

bool HttpAcceptParser::compareContentTypes(const ParsedContentType &a, const ParsedContentType &b)
{
    // Sort by quality score
    if (a.qvalue != b.qvalue)
    {
        return a.qvalue > b.qvalue;
    }

    // Sort by type
    if (a.type != b.type)
    {
        if (a.type == "*")
        {
            return true;
        }

        if (b.type == "*")
        {
            return false;
        }

        return a.order < b.order;
    }

    // Sort by subtype
    if (a.subtype != b.subtype)
    {
        if (a.subtype == "*")
        {
            return true;
        }

        if (b.subtype == "*")
        {
            return false;
        }

        return a.order < b.order;
    }

    // Sort by order
    return a.order < b.order;
}

std::string HttpAcceptParser::getPreferableContentType(const std::vector<ParsedContentType> &acceptedContentTypes, const std::vector<std::string> &availableContentTypes)
{
    std::vector<ParsedContentType> selectedContentTypes;

    int order = 0;
    for (auto contentTypeStr : availableContentTypes)
    {
        stringToLower(trim(contentTypeStr));
        ParsedContentType selectedContentType{contentTypeStr, "", "", 0, order};
        auto indexSlash = contentTypeStr.find('/');
        if (indexSlash == std::string::npos)
        {
            // Invalid content type format.
            continue;
        }
        selectedContentType.type = std::string(contentTypeStr.begin(), contentTypeStr.begin() + indexSlash);
        selectedContentType.subtype = std::string(contentTypeStr.begin() + indexSlash + 1, contentTypeStr.end());

        bool matchFound = false;
        for (const auto &acceptedContentType : acceptedContentTypes)
        {
            if ((acceptedContentType.type == selectedContentType.type) && ((acceptedContentType.subtype == selectedContentType.subtype) || ((acceptedContentType.subtype == "*") && !matchFound)))
            {
                // Match 'type/subtype' or 'type/*'
                selectedContentType.qvalue = acceptedContentType.qvalue;
                matchFound = true;
            }
            else if ((acceptedContentType.type == "*") && (!matchFound))
            {
                // Match '*/*'
                selectedContentType.qvalue = acceptedContentType.qvalue;
            }
        }
        selectedContentTypes.push_back(selectedContentType);
        order++;
    }

    // Sort selected content types by score.
    std::sort(selectedContentTypes.begin(), selectedContentTypes.end(), compareContentTypes);

    // Get the first selected content type (wich is the content type with the best score).
    // If no content types has been selected then return the first available content type.
    if (!selectedContentTypes.empty())
    {
        return selectedContentTypes.front().range;
    }
    else if (!availableContentTypes.empty())
    {
        return availableContentTypes.front();
    }

    return std::string();
}
