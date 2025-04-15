#pragma once

#include <string>
#include <vector>

#include "forma/core.hh"

namespace forma
{

	enum class TokenType
	{
	    Text,
	    BeginCode, EndCode,
	    BeginCodeTrim, EndCodeTrim,
	    Ident,
	    Dot,
	    Comma,
	    Pipe,
	    LeftParen, RightParen,
	    Hash,
	    Slash,
	    QuestionMark,
	    Eof,
	    KeywordIf, KeywordRange, KeywordEnd, KeywordInclude
	};
	template<typename S>
	S& operator<<(S& s, TokenType tt)
	{
		switch(tt)
		{
#define C(x) case TokenType::x: s << #x; break
			C(Text);
			C(BeginCode);
			C(EndCode);
			C(BeginCodeTrim);
			C(EndCodeTrim);
			C(Ident);
			C(Dot);
			C(Comma);
			C(Pipe);
			C(LeftParen);
			C(RightParen);
			C(Hash);
			C(Slash);
			C(QuestionMark);
			C(Eof);
			C(KeywordIf);
			C(KeywordRange);
			C(KeywordEnd);
			C(KeywordInclude);
#undef C
		default:
			s << "<???>";
			break;
		}

		return s;
	}

	struct Token
	{
	    TokenType Type;
	    std::string Lexeme;
	    Location Location;
	    std::string Value;

		Token(TokenType t, std::string l, forma::Location lo, std::string v);
		Token withType(TokenType t) const;
		Token withValue(const std::string& v) const;
	};


	struct ScannerLocation
	{
	    int Line;
	    int Offset;
	    int Index;

		ScannerLocation(int l, int o, int i);
	};

	using ScanResult = std::pair<std::vector<Token>, std::vector<Error>>;
	ScanResult Scan(const std::string& file, const std::string& source);
};