/* -*- c++ -*- */

#ifndef HTTP_ACCEPT_PARSER_H
#define HTTP_ACCEPT_PARSER_H

#include <vector>
#include <string>

/**
 * Helper class for parsing the HTTP 'Accept' header.
 */
class HttpAcceptParser
{
public:

    /**
     * Returns a content type from a list of available content types according
     * to the preferences specified in a HTTP 'Accept' header. 
     * 
     * @param[in] acceptValue value of the 'Accept' header.
     * @param[in] availableContentTypes list of available content types.
     * 
     * @return the selected content type.
     */
    static std::string parse(const std::string & acceptValue, const std::vector<std::string> & availableContentTypes);

private:

    /**
     * Constructor.
     */
    HttpAcceptParser()
    {
    }

    /**
     * Destructor.
     */
    ~HttpAcceptParser( )
    {
    }

    /**
     * @brief Representation of a Mime Type containing additional information to facilitate
     * the content type negotiation when a HTTP requests arrives.
     */
    struct ParsedContentType
    {
        std::string range;
        std::string type;
        std::string subtype;
        float       qvalue;
        int         order;
    };

    /**
     * Converts a numeric string to its respective float value. 
     * 
     * @param[in] s numeric string containing a float number.
     * @param[out] f destination of the converted float value.
     * 
     * @return False if the conversion fails. Returns True otherwise.
     */
    static bool stringToFloat(const std::string &s, float *f);

    /**
     * Strip specified characters from the end of a string.
     * 
     * @param[in,out] s string that will be trimmed.
     * @param[in] t list of all characters that will be stripped.
     * 
     * @return the modified string.
     */
    static std::string &rtrim(std::string &s, const char *t);

    /**
     * Strip specified characters from the beginning of a string.
     * 
     * @param[in,out] s string that will be trimmed.
     * @param[in] t list of all characters that will be stripped.
     * 
     * @return the modified string.
     */
    static std::string &ltrim(std::string &s, const char *t);

    /**
     * Strip whitespace (and other characters) from the beginning and end of a string.
     * 
     * @param[in,out] s string that will be trimmed.
     * 
     * @return the modified string.
     */
    static std::string &trim(std::string &s);

    /**
     * Make a string lowercase.
     * 
     * @param[in,out] s string that will be converted.
     * 
     * @return the string with all alphabetic characters converted to lowercase.
     */
    static std::string &stringToLower(std::string &s);

    /**
     * Determines wheter a content type is preferrable over another content type.
     * 
     * @param[in] a the content type to be compared from.
     * @param[in] b the content type to be compared to.
     * 
     * @return True if the content type 'a' is preferrable over the content type 'b'. Returns False otherwise.
     */
    static bool compareContentTypes(const ParsedContentType &a, const ParsedContentType &b);

    /**
     * Returns the preferable content type from a list of available content types
     * according to a list of accepted content types.
     * 
     * @param[in] acceptedContentTypes list of accepted content types with normalized weights.
     * @param[in] availableContentTypes list of available content types ordeder by preference.
     * 
     * @return the preferable and accepted content type from the list of available content types.
     */
    static std::string getPreferableContentType(const std::vector<ParsedContentType> &acceptedContentTypes, const std::vector<std::string> &availableContentTypes);
};

#endif // HTTP_ACCEPT_PARSER_H