#pragma once
#include <string>

namespace sanghavimart::util {

/// Minimal HTML escaper for server-rendered / reflected user content.
/// Frontend must use escapeHtml() in api.js; this covers any C++-rendered path.
inline std::string HtmlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c;
        }
    }
    return out;
}

}  // namespace sanghavimart::util
