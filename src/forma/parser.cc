#include "forma/parser.hh"

#include "forma/core.hh"

#include <optional>
#include <array>

namespace forma
{
namespace node
{
	Text::Text(std::string v, forma::Location l)
		: Value(v)
		, Location(l)
	{
	}

	Text* Text::AsText()
	{
		return this;
	}

	Attribute::Attribute(std::string n, forma::Location l)
		: Name(n)
		, Location(l)
	{
	}

	Attribute* Attribute::AsAttribute()
	{
		return this;
	}

	Iterate::Iterate(std::string n, std::shared_ptr<Node> b, forma::Location l)
		: Name(n)
		, Body(b)
		, Location(l)
	{
	}

	Iterate* Iterate::AsIterate()
	{
		return this;
	}

	If::If(std::string n, std::shared_ptr<Node> b, forma::Location l)
		: Name(n)
		, Body(b)
		, Location(l)
	{
	}

	If* If::AsIf()
	{
		return this;
	}

	FunctionCall::FunctionCall(std::string n, Func f, std::shared_ptr<Node> a, forma::Location l)
		: Name(n)
		, Function(f)
		, Arg(a)
		, Location(l)
	{
	}

	FunctionCall* FunctionCall::AsFunctionCall()
	{
		return this;
	}

	Group::Group(std::vector<std::shared_ptr<Node>> n, forma::Location l)
		: Nodes(n)
		, Location(l)
	{
	}

	Group* Group::AsGroup()
	{
		return this;
	}
}  //  namespace node

template<typename K, typename V>
std::vector<K> GetKeys(const std::unordered_map<K, V>& m)
{
	std::vector<K> ks;
	for (const auto& [key, _]: m)
	{
		ks.emplace_back(key);
	}
	return ks;
}

std::vector<Token> TrimTextTokens(const std::vector<Token>& tokens)
{
	std::vector<Token> r;

	std::optional<Token> lastToken = std::nullopt;
	for (const auto& tok: tokens)
	{
		switch (tok.Type)
		{
		case TokenType::BeginCodeTrim:
			if (lastToken.has_value() && lastToken->Type == TokenType::Text)
			{
				r.emplace_back(lastToken->withValue(strings::TrimEnd(lastToken->Value)));
			}

			lastToken = tok.withType(TokenType::BeginCode);
			break;
		case TokenType::Text:
			if (lastToken.has_value() && lastToken->Type == TokenType::EndCodeTrim)
			{
				r.emplace_back(lastToken->withType(TokenType::EndCode));
				lastToken = tok.withValue(strings::TrimStart(tok.Value));
				break;
			}
		default:
			if (lastToken.has_value())
			{
				r.emplace_back(*lastToken);
			}

			lastToken = tok;
			break;
		}
	}

	if (lastToken.has_value())
	{
		r.emplace_back(*lastToken);
	}

	return r;
}

std::vector<Token> TrimEmptyStartEnd(const std::vector<Token>& tokens)
{
	std::vector<Token> r;
	std::optional<Token> lastToken = std::nullopt;
	for (const auto& tok: tokens)
	{
		if (lastToken.has_value() && lastToken->Type == TokenType::BeginCode
			&& tok.Type == TokenType::EndCode)
		{
			lastToken = std::nullopt;
			continue;
		}

		if (lastToken.has_value())
		{
			r.emplace_back(*lastToken);
		}

		lastToken = tok;
	}

	if (lastToken.has_value())
	{
		r.emplace_back(*lastToken);
	}

	return r;
}

std::vector<Token> TransformSingleCharsToKeywords(const std::vector<Token>& tokens)
{
	auto eatIdent = false;
	std::vector<Token> r;
	std::optional<Token> lastToken = std::nullopt;
	for (const auto& tok: tokens)
	{
		if (tok.Type == TokenType::Ident && eatIdent)
		{
			eatIdent = false;
			continue;
		}

		if (tok.Type == TokenType::Slash && lastToken.has_value()
			&& lastToken->Type == TokenType::BeginCode)
		{
			r.emplace_back(*lastToken);
			lastToken = tok.withType(TokenType::KeywordEnd);
			eatIdent = true;
		}
		else if (tok.Type == TokenType::Hash && lastToken.has_value()
				 && lastToken->Type == TokenType::BeginCode)
		{
			r.emplace_back(*lastToken);
			lastToken = tok.withType(TokenType::KeywordRange);
		}
		else if (tok.Type == TokenType::QuestionMark && lastToken.has_value()
				 && lastToken->Type == TokenType::BeginCode)
		{
			r.emplace_back(*lastToken);
			lastToken = tok.withType(TokenType::KeywordIf);
		}
		else
		{
			if (lastToken.has_value())
			{
				r.emplace_back(*lastToken);
			}

			lastToken = tok;
		}
	}

	if (lastToken.has_value())
	{
		r.emplace_back(*lastToken);
	}

	return r;
}

// todo(Gustav): remove exceptions!
struct ParseError : std::runtime_error
{
	ParseError()
		: std::runtime_error("parse error")
	{
	}
};

struct Parser
{
	std::vector<Token> tokens;

	std::unordered_map<std::string, FuncGenerator> functions;
	DirectoryInfo* includeDir;
	std::string defaultExtension;
	VfsRead* vfs;

	int current = 0;
	std::vector<Error> errors;

	Parser(
		std::vector<Token> itok,
		std::unordered_map<std::string, FuncGenerator> f,
		DirectoryInfo* i,
		std::string d,
		VfsRead* v
	)
		: tokens(TransformSingleCharsToKeywords(TrimEmptyStartEnd(TrimTextTokens(itok))))
		, functions(f)
		, includeDir(i)
		, defaultExtension(d)
		, vfs(v)
	{
	}

	ParseResult parse()
	{
		auto rootNode = ParseGroup();
		if (! IsAtEnd())
		{
			ReportError(Peek().Location, ExpectedMessage("EOF"));
		}

		if (errors.size() == 0)
		{
			return {rootNode, errors};
		}
		return {std::make_shared<node::Text>("Parsing failed", UnknownLocation()), errors};
	}

	std::shared_ptr<Node> ParseGroup()
	{
		auto start = Peek().Location;
		auto nodes = std::vector<std::shared_ptr<Node>>();
		while (! IsAtEnd()
			   && ! (Peek().Type == TokenType::BeginCode && PeekNext() == TokenType::KeywordEnd))
		{
			try
			{
				ParseNode(nodes);
			}
			catch (const ParseError&)
			{
				Synchronize();
			}
		}

		return std::make_shared<node::Group>(nodes, start);
	}

	ParseError ReportError(Location loc, std::string message)
	{
		errors.push_back(Error(loc, message));
		return ParseError();
	}

	bool Match(TokenType type)
	{
		return MatchMany(std::array<TokenType, 1>{type});
	}

	template<std::size_t Size>
	bool MatchMany(std::array<TokenType, Size> types)
	{
		for (const auto& t: types)
		{
			// todo(Gustav): hrm???
			if (Check(t) == false) return false;
		}

		Advance();
		return true;
	}

	bool Check(TokenType type)
	{
		if (IsAtEnd()) return false;
		return Peek().Type == type;
	}

	Token Advance()
	{
		if (! IsAtEnd()) current++;
		return Previous();
	}

	bool IsAtEnd()
	{
		return Peek().Type == TokenType::Eof;
	}

	Token Peek()
	{
		return tokens[current];
	}

	TokenType PeekNext()
	{
		if (current + 1 >= tokens.size()) return TokenType::Eof;
		return tokens[current + 1].Type;
	}

	Token Previous()
	{
		return tokens[current - 1];
	}

	void Synchronize()
	{
		Advance();

		while (! IsAtEnd())
		{
			if (Previous().Type == TokenType::EndCode) return;

			switch (Peek().Type)
			{
			case TokenType::Text: return;
			default: break;
			}

			Advance();
		}
	}

	static std::string TokenToMessage(Token token)
	{
		std::string value
			= token.Type == TokenType::Text ? std::string{""} : Fmt{} << ": " << token.Lexeme;
		return Fmt{} << token.Type << value;
	}

	Token Consume(TokenType type, std::string message)
	{
		if (Check(type)) return Advance();

		throw ReportError(Peek().Location, message);
	}

	FuncArgument ParseFunctionArg()
	{
		if (Peek().Type != TokenType::Ident)
		{
			throw ReportError(Peek().Location, ExpectedMessage("identifier"));
		}

		auto arg = Advance();
		return FuncArgument(arg.Location, arg.Value);
	}

	std::string ExtractAttributeName()
	{
		auto ident = Consume(TokenType::Ident, ExpectedMessage("IDENT"));
		return ident.Value;
	}

	std::string ExpectedMessage(std::string what)
	{
		return Fmt{} << "Expected " << what << " but found " << TokenToMessage(Peek());
	}

	void ParseNode(std::vector<std::shared_ptr<Node>>& nodes)
	{
		switch (Peek().Type)
		{
		case TokenType::BeginCode:
			{
				auto start = Peek().Location;
				Advance();

				if (Match(TokenType::KeywordRange))
				{
					auto attribute = ExtractAttributeName();
					Consume(TokenType::EndCode, ExpectedMessage("}}"));

					auto group = ParseGroup();
					Consume(TokenType::BeginCode, ExpectedMessage("{{"));
					Consume(TokenType::KeywordEnd, ExpectedMessage("keyword end"));
					Consume(TokenType::EndCode, ExpectedMessage("}}"));

					nodes.emplace_back(std::make_shared<node::Iterate>(attribute, group, start));
				}
				else if (Match(TokenType::KeywordIf))
				{
					auto attribute = ExtractAttributeName();
					Consume(TokenType::EndCode, ExpectedMessage("}}"));

					auto group = ParseGroup();
					Consume(TokenType::BeginCode, ExpectedMessage("{{"));
					Consume(TokenType::KeywordEnd, ExpectedMessage("keyword end"));
					Consume(TokenType::EndCode, ExpectedMessage("}}"));

					nodes.emplace_back(std::make_shared<node::If>(attribute, group, start));
				}
				else if (Match(TokenType::KeywordInclude))
				{
					auto name = Consume(TokenType::Ident, ExpectedMessage("IDENT"));
					auto includeLocation = Peek().Location;
					Consume(TokenType::EndCode, ExpectedMessage("}}"));

					auto firstFile = includeDir->GetFile(name.Value);
					auto file = firstFile;
					auto secondFile = firstFile;
					if (vfs->Exists(file) == false)
					{
						secondFile = includeDir->GetFile(name.Value + defaultExtension);
						file = secondFile;
					}

					if (vfs->Exists(file) == false)
					{
						ReportError(
							includeLocation,
							Fmt{} << "Unable to open file: tried " << firstFile << " and "
								  << secondFile
						);
					}
					else
					{
						auto source = vfs->ReadAllText(file);
						auto [scannerTokens, lexerErrors] = Scan(file, source);
						if (lexerErrors.size() > 0)
						{
							ReportError(includeLocation, "included from here...");
							for (auto e: lexerErrors)
							{
								ReportError(e.Location, e.Message);
							}
							return;
						}

						auto [node, parseErrors]
							= Parse(scannerTokens, functions, includeDir, defaultExtension, vfs);
						if (parseErrors.size() > 0)
						{
							ReportError(includeLocation, "included from here...");
							for (auto e: parseErrors)
							{
								ReportError(e.Location, e.Message);
							}

							return;
						}

						nodes.emplace_back(node);
					}
				}
				else
				{
					ParseAttributeToEnd(nodes);
				}
			}
			break;
		case TokenType::Text:
			{
				auto text = Advance();
				nodes.emplace_back(std::make_shared<node::Text>(text.Value, text.Location));
			}
			break;
		default:
			throw ReportError(
				Peek().Location, Fmt{} << "Unexpected token " << TokenToMessage(Peek())
			);
		}
	}

	void ParseAttributeToEnd(std::vector<std::shared_ptr<Node>>& nodes)
	{
		auto start = Peek().Location;
		std::shared_ptr<Node> node
			= std::make_shared<node::Attribute>(ExtractAttributeName(), start);

		while (Peek().Type == TokenType::Pipe)
		{
			Advance();
			auto name = Consume(TokenType::Ident, ExpectedMessage("function name"));
			auto arguments = std::vector<FuncArgument>();

			if (Match(TokenType::LeftParen))
			{
				while (Peek().Type != TokenType::RightParen && ! IsAtEnd())
				{
					arguments.emplace_back(ParseFunctionArg());

					if (Peek().Type != TokenType::RightParen)
					{
						Consume(
							TokenType::Comma,
							ExpectedMessage("comma for the next function argument")
						);
					}
				}

				Consume(TokenType::RightParen, ExpectedMessage(") to end function"));
			}

			if (auto funcGenerator = functions.find(name.Value); funcGenerator != functions.end())
			{
				auto [func, funcParseErrors] = funcGenerator->second(name.Location, arguments);
				if (funcParseErrors.empty() == false)
				{
					for (const auto& err: funcParseErrors)
					{
						ReportError(err.Location, err.Message);
					}
				}
				node = std::make_shared<node::FunctionCall>(name.Value, func, node, name.Location);
			}
			else
			{
				ReportError(
					name.Location,
					Fmt{} << "Unknown function named " << name.Value << ": "
						  << MatchStrings(name.Value, GetKeys(functions))
				);
			}
		}
		nodes.emplace_back(node);

		Consume(TokenType::EndCode, ExpectedMessage("end token"));
	}
};

ParseResult Parse(
	std::vector<Token> itok,
	std::unordered_map<std::string, FuncGenerator> functions,
	DirectoryInfo* includeDir,
	std::string defaultExtension,
	VfsRead* vfs
)
{
	Parser parser{itok, functions, includeDir, defaultExtension, vfs};
	return parser.parse();
}
}  //  namespace forma
