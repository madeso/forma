#include "forma/core.hh"

#include <algorithm>

namespace forma
{
std::vector<Error> NoErrors()
{
	return {};
}

Location UnknownLocation()
{
	return {{"unknown-file.txt"}, -1, -1};
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
	ss << "could be: ";
	bool first = true;
	for (const auto& c: candidates)
	{
		if (first)
			first = false;
		else
			ss << ' ';
		ss << c;
	}
	return ss.str();
#endif
}

namespace strings
{
	bool is_letter(char c)
	{
		switch (c)
		{
		case 'A':
		case 'a':
		case 'B':
		case 'b':
		case 'C':
		case 'c':
		case 'D':
		case 'd':
		case 'E':
		case 'e':
		case 'F':
		case 'f':
		case 'G':
		case 'g':
		case 'H':
		case 'h':
		case 'I':
		case 'i':
		case 'J':
		case 'j':
		case 'K':
		case 'k':
		case 'L':
		case 'l':
		case 'M':
		case 'm':
		case 'N':
		case 'n':
		case 'O':
		case 'o':
		case 'P':
		case 'p':
		case 'Q':
		case 'q':
		case 'R':
		case 'r':
		case 'S':
		case 's':
		case 'T':
		case 't':
		case 'U':
		case 'u':
		case 'V':
		case 'v':
		case 'W':
		case 'w':
		case 'X':
		case 'x':
		case 'Y':
		case 'y':
		case 'Z':
		case 'z': return true;
		default: return false;
		}
	}

	char to_upper(char c)
	{
		switch (c)
		{
		case 'a': return 'A';
		case 'b': return 'B';
		case 'c': return 'C';
		case 'd': return 'D';
		case 'e': return 'E';
		case 'f': return 'F';
		case 'g': return 'G';
		case 'h': return 'H';
		case 'i': return 'I';
		case 'j': return 'J';
		case 'k': return 'K';
		case 'l': return 'L';
		case 'm': return 'M';
		case 'n': return 'N';
		case 'o': return 'O';
		case 'p': return 'P';
		case 'q': return 'Q';
		case 'r': return 'R';
		case 's': return 'S';
		case 't': return 'T';
		case 'u': return 'U';
		case 'v': return 'V';
		case 'w': return 'W';
		case 'x': return 'X';
		case 'y': return 'Y';
		case 'z': return 'Z';
		default: return c;
		}
	}

	char to_lower(char c)
	{
		switch (c)
		{
		case 'A': return 'a';
		case 'B': return 'b';
		case 'C': return 'c';
		case 'D': return 'd';
		case 'E': return 'e';
		case 'F': return 'f';
		case 'G': return 'g';
		case 'H': return 'h';
		case 'I': return 'i';
		case 'J': return 'j';
		case 'K': return 'k';
		case 'L': return 'l';
		case 'M': return 'm';
		case 'N': return 'n';
		case 'O': return 'o';
		case 'P': return 'p';
		case 'Q': return 'q';
		case 'R': return 'r';
		case 'S': return 's';
		case 'T': return 't';
		case 'U': return 'u';
		case 'V': return 'v';
		case 'W': return 'w';
		case 'X': return 'x';
		case 'Y': return 'y';
		case 'Z': return 'z';
		default: return c;
		}
	}

	bool is_whitespace(char c)
	{
		switch (c)
		{
		case ' ':
		case '\t':
		case '\r':
		case '\n': return true;
		default: return false;
		}
	}

	std::string default_space()
	{
		return " \t\n\r";
	}

	std::string TrimStart(const std::string& s, const std::string& spaces)
	{
		size_t start = s.find_first_not_of(spaces);
		return (start == std::string::npos) ? "" : s.substr(start);
	}

	std::string TrimEnd(const std::string& s, const std::string& spaces)
	{
		size_t end = s.find_last_not_of(spaces);
		return (end == std::string::npos) ? "" : s.substr(0, end + 1);
	}

	std::string Capitalize(const std::string& p, bool alsoFirstChar)
	{
		auto cap = alsoFirstChar;
		std::ostringstream sb;
		for (char h: p)
		{
			auto c = to_lower(h);
			if (is_letter(c) && cap)
			{
				c = to_upper(c);
				cap = false;
			}
			if (is_whitespace(c)) cap = true;
			sb << c;
		}
		return sb.str();
	}

	std::string ToLower(const std::string& args)
	{
		std::ostringstream ss;
		for (char c: args)
		{
			ss << to_lower(c);
		}
		return ss.str();
	}

	std::string ToUpper(const std::string& args)
	{
		std::ostringstream ss;
		for (char c: args)
		{
			ss << to_upper(c);
		}
		return ss.str();
	}

	std::string ToTitleCase(const std::string& args)
	{
		return Capitalize(args, true);
	}

	std::string Trim(const std::string& s, const std::string& space)
	{
		return TrimStart(TrimEnd(s, space), space);
	}

	std::string PadLeft(const std::string& s, int count, char c)
	{
		const auto si = s.length();
		if (si >= count) return s;
		const auto sp = std::string(count - si, c);
		return sp + s;
	}

	std::string Replace(const std::string& arg, const std::string& lhs, const std::string& rhs)
	{
		std::string r = arg;
		auto start = 0;
		while (true)
		{
			auto found = r.find(lhs, start);
			if (found == std::string::npos) return r;
			r.replace(found, lhs.length(), rhs);
			start = found + rhs.length();
		}
	}

	std::string Substring(const std::string& arg, int start, int count)
	{
		// todo(Gustav): how do we want to handle negative indices... index from the end?
		return arg.substr(start, count);
	}
}  //  namespace strings
}  //  namespace forma
