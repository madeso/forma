#include "forma/scanner.hh"

#include <optional>

namespace forma
{
Token::Token(TokenType t, std::string l, forma::Location lo, std::string v)
	: Type(t)
	, Lexeme(l)
	, Location(lo)
	, Value(v)
{
}

Token Token::withType(TokenType new_type) const
{
	return {new_type, Lexeme, Location, Value};
}

Token Token::withValue(const std::string& new_value) const
{
	return {Type, Lexeme, Location, new_value};
}

ScannerLocation::ScannerLocation(int l, int o, int i)
	: Line(l)
	, Offset(o)
	, Index(i)
{
}

ScannerLocation with_index_offset(const ScannerLocation& l, int i, int o)
{
	return {l.Line, o, i};
}

struct Scanner
{
	std::string file;
	std::string source;
	ScannerLocation start;
	ScannerLocation current;
	bool insideCodeBlock;
	std::vector<Error> errors;
	std::vector<Token> ret;

	Scanner(const std::string& f, const std::string& s)
		: file(f)
		, source(s)
		, start(ScannerLocation{1, 0, 0})
		, current(start)
		, insideCodeBlock(false)
	{
	}

	ScanResult scan()
	{
		while (false == IsAtEnd())
		{
			start = current;
			auto tokens = ScanToken();
			for (const auto& tok: tokens)
			{
				ret.emplace_back(tok);
			}
		}
		ret.emplace_back(Token{TokenType::Eof, "", Location{file, current.Line, current.Offset}, ""}
		);

		if (errors.empty() == false)
		{
			ret.clear();
		}
		return {ret, errors};
	}

	void ReportError(const Location& loc, const std::string& message)
	{
		errors.push_back(Error{loc, message});
	}

	std::vector<Token> ScanToken()
	{
		if (insideCodeBlock)
		{
			const auto tok = ScanCodeToken();
			if (tok.has_value()) return {*tok};
		}
		else
		{
			while (insideCodeBlock == false && false == IsAtEnd())
			{
				auto beforeStart = current;
				auto c = Advance();
				if (c == '{' && Match('{'))
				{
					auto beginType = TokenType::BeginCode;
					if (Peek() == '-')
					{
						Advance();
						beginType = TokenType::BeginCodeTrim;
					}
					auto afterStart = current;
					auto text = CreateToken(TokenType::Text, std::nullopt, start, beforeStart);

					std::vector<Token> ret;
					if (text.Value.length() > 0)
					{
						ret.emplace_back(text);
					}
					insideCodeBlock = true;

					ret.emplace_back(CreateToken(beginType, std::nullopt, beforeStart, current));
					return ret;
				}
			}

			if (IsAtEnd())
			{
				auto text = CreateToken(TokenType::Text);
				if (text.Value.length() > 0)
				{
					return {text};
				}
			}
		}

		return {};
	}

	std::optional<Token> ScanCodeToken()
	{
		const auto c = Advance();
		switch (c)
		{
		case '-':
			if (! Match('}'))
			{
				ReportError(GetStartLocation(), "Detected rouge -");
				return std::nullopt;
			}

			if (! Match('}'))
			{
				ReportError(GetStartLocation(), "Detected rouge -}");
				return std::nullopt;
			}

			insideCodeBlock = false;
			return CreateToken(TokenType::EndCodeTrim);
		case '}':
			if (! Match('}'))
			{
				ReportError(GetStartLocation(), "Detected rouge {");
				return std::nullopt;
			}

			insideCodeBlock = false;
			return CreateToken(TokenType::EndCode);
		case '|': return CreateToken(TokenType::Pipe);
		case ',': return CreateToken(TokenType::Comma);
		case '(': return CreateToken(TokenType::LeftParen);
		case ')': return CreateToken(TokenType::RightParen);
		case '#': return CreateToken(TokenType::Hash);
		case '.': return CreateToken(TokenType::Dot);
		case '?': return CreateToken(TokenType::QuestionMark);

		case '/':
			if (! Match('*'))
			{
				return CreateToken(TokenType::Slash);
			}
			while (! (Peek() == '*' && PeekNext() == '/') && ! IsAtEnd())
			{
				Advance();
			}

			// skip * and /
			if (! IsAtEnd())
			{
				Advance();
				Advance();
			}

			return std::nullopt;

		case '"':
			{
				while (Peek() != '"' && ! IsAtEnd())
				{
					Advance();
				}

				if (IsAtEnd())
				{
					ReportError(GetStartLocation(), "Unterminated std::string.");
					return std::nullopt;
				}

				// The closing ".
				Advance();

				// Trim the surrounding quotes.
				auto value = source.substr(start.Index + 1, current.Index - start.Index - 2);
				return CreateToken(TokenType::Ident, value);
			}

		case ' ':
		case '\r':
		case '\n':
		case '\t': return std::nullopt;

		default:
			if (IsDigit(c))
			{
				while (IsDigit(Peek()))
					Advance();

				// Look for a fractional part.
				if (Peek() == '.' && IsDigit(PeekNext()))
				{
					// Consume the "."
					Advance();

					while (IsDigit(Peek()))
						Advance();
				}

				return CreateToken(TokenType::Ident);
			}
			else if (IsAlpha(c))
			{
				while (IsAlphaNumeric(Peek()))
					Advance();
				auto ident = CreateToken(TokenType::Ident);

				// check keywords
				if (ident.Value == "if")
				{
					ident.Type = TokenType::KeywordIf;
					return ident;
				}
				if (ident.Value == "range")
				{
					ident.Type = TokenType::KeywordRange;
					return ident;
				}
				if (ident.Value == "end")
				{
					ident.Type = TokenType::KeywordEnd;
					return ident;
				}
				if (ident.Value == "include")
				{
					ident.Type = TokenType::KeywordInclude;
					return ident;
				}
				return ident;
			}
			else
			{
				ReportError(GetStartLocation(), Fmt{} << "Unexpected character " << c);
				return std::nullopt;
			}
		}
	}

	static bool IsDigit(char c)
	{
		return c >= '0' && c <= '9';
	}

	static bool IsAlpha(char c)
	{
		return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
	}

	static bool IsAlphaNumeric(char c)
	{
		return IsAlpha(c) || IsDigit(c);
	}

	ScannerLocation NextChar()
	{
		return with_index_offset(current, current.Index + 1, current.Offset + 1);
	}

	bool Match(char expected)
	{
		if (IsAtEnd())
		{
			return false;
		}
		if (source[current.Index] != expected)
		{
			return false;
		}

		current = NextChar();
		return true;
	}

	char Peek()
	{
		return IsAtEnd() ? '\0' : source[current.Index];
	}

	char PeekNext()
	{
		auto suggestedIndex = current.Index + 1;
		return suggestedIndex >= source.size() ? '\0' : source[suggestedIndex];
	}

	char Advance()
	{
		const auto c = source[current.Index];
		current = NextChar();
		if (c == '\n')
		{
			const auto c = current;
			current.Offset = 0;
			current.Line = c.Line + 1;
		}
		return c;
	}

	Location GetStartLocation(std::optional<ScannerLocation> stt = std::nullopt)
	{
		const auto st = stt.value_or(start);
		return Location(file, st.Line, st.Offset);
	}

	Token CreateToken(
		TokenType tt,
		std::optional<std::string> value = std::nullopt,
		std::optional<ScannerLocation> begin = std::nullopt,
		std::optional<ScannerLocation> end = std::nullopt
	)
	{
		const auto st = begin.value_or(start);
		const auto cu = end.value_or(current);
		const auto text = source.substr(st.Index, cu.Index - st.Index);
		return Token{tt, text, GetStartLocation(st), value.value_or(text)};
	}

	bool IsAtEnd()
	{
		return current.Index >= source.length();
	}
};

std::pair<std::vector<Token>, std::vector<Error>> Scan(
	const std::string& file, const std::string& source
)
{
	auto scanner = Scanner{file, source};
	return scanner.scan();
}
}  //  namespace forma
