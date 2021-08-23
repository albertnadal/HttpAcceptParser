# HttpAcceptParser
C++11 class for parsing HTTP 'Accept' headers. Implemented according to the RFC 7231 and 2616.

Example:
```cpp
const auto selectedContentType = HttpAcceptParser::parse("*/*;q=0.5, text/xml;q=0.55, image/png;q=0", { "application/json", "image/png", "text/xml", "text/plain" });
assert(selectedContentType == "text/xml");
```