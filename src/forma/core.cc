#include "forma/core.hh"

namespace forma
{
    std::vector<Error> NoErrors()
    {
        return {};
    }

    Location UnknownLocation()
    {
        return { { "unknown-file.txt" }, -1, -1 };
    }

    std::string MatchStrings(std::string& name, const std::vector<std::string>& candidates)
    {
#if 0
        auto all = candidates.ToImmutableArray();
        auto matches = EditDistance.ClosestMatches(name, 10, all).ToImmutableArray();

        return matches.Length > 0
            ? $"did you mean {ToArrayString(matches)} of {all.Length}: {ToArrayString(all)}"
            : $"No match in {all.Length}: {ToArrayString(all)}"
            ;

        static std::string ToArrayString(std::vector<std::string> candidates)
        {
            auto s = std::string.Join(", ", candidates);
            return $"[{s}]";
        }
#else
        std::ostringstream ss;
        ss << "Missing " << name << ", could be: ";
        bool first = true;
        for (const auto& c : candidates)
        {
            if (first) first = false;
            else ss << ' ';
            ss << c;
        }
        return ss.str();
#endif
    }

    namespace strings
    {
        std::string TrimStart(const std::string& s)
        {
            size_t start = s.find_first_not_of(" \t\n\r");
            return (start == std::string::npos) ? s : s.substr(start);
        }

        std::string TrimEnd(const std::string& s)
        {
            size_t end = s.find_last_not_of(" \t\n\r");
            return (end == std::string::npos) ? s : s.substr(0, end + 1);
        }
    }
}