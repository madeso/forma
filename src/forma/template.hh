// forma template engine

// ReSharper disable CppInconsistentNaming
// ReSharper disable IdentifierTypo
// ReSharper disable CommentTypo
// ReSharper disable CppClangTidyCppcoreguidelinesSpecialMemberFunctions
// ReSharper disable CppClassCanBeFinal
// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <cassert>
#include <sstream>

/*

  API:
    auto (generator, error) = Template.Parse(...);
    string ret = generator(myClass);

  Template syntax:
    {{ prop }} {{- "also prop, trim printable spaces" -}}
    {{prop | function | function(with_arguments)}}
    {{include file}} {{include "file/with.extension"}}
    {{#list}}repeated{{/list}} {{range also_list}}repeated{{end}}
    {{if bool_prop}}perhaps{{end}}

*/

// todo(Gustav): add foo.bar and .foobar accessors to access subdicts and global dicts
// todo(Gustav): add documentation to properties and functions so error messages can be more helpful, this also means we can generate documentation for the current build
// todo(Gustav): should return values always be strings? properties return datetime that format functions could format


namespace forma
{
    struct Location
    {
        std::string File;
        int Line;
        int Offset;
    };

    struct Error
    {
        Location Location;
        std::string Message;
    };

    struct FuncArgument
    {
        Location Location;
        std::string Argument;
    };

    // apply dynamic std::string function and return result
    using Func = std::function<std::string(std::string)>;

    // parse arguments, return function call or error
    using FuncGeneratorResult = std::pair<Func, std::vector<Error>>;
    using FuncGenerator = std::function<FuncGeneratorResult(Location call, std::vector<FuncArgument> arguments)>;

    struct Text;
    struct Attribute;
    struct Iterate;
    struct If;
    struct FunctionCall;
    struct Group;

    struct Node
    {
        virtual ~Node() = default;

        virtual Text* AsText() { return nullptr; }
        virtual Attribute* AsAttribute() { return nullptr; }
        virtual Iterate* AsIterate() { return nullptr; }
        virtual If* AsIf() { return nullptr; }
        virtual FunctionCall* AsFunctionCall() { return nullptr; }
        virtual Group* AsGroup() { return nullptr; }
    };

    struct Text : Node
    {
        std::string Value;
        Location Location;

        Text* AsText() override
        {
            return this;
        }
    };

    struct Attribute : Node
    {
        std::string Name;
        Location Location;

        Attribute* AsAttribute() override
        {
            return this;
        }
    };
    struct Iterate : Node
    {
        std::string Name;
        Node Body;
        Location Location;

        Iterate* AsIterate() override
        {
            return this;
        }
    };
    struct If : Node
    {
        std::string Name;
        Node Body;
        Location Location;

        If* AsIf() override
        {
            return this;
        }
    };
    struct FunctionCall : Node
    {
        std::string Name;
        Func Function;
        Node Arg;
        Location Location;

        FunctionCall* AsFunctionCall() override
        {
            return this;
        }
    };
    struct Group : Node
    {
        std::vector<std::shared_ptr<Node>> Nodes;
        Location Location;

        Group* AsGroup() override
        {
            return this;
        }
    };

    std::vector<Error> NoErrors()
    {
        return {};
    }

    Location UnknownLocation()
    {
        return { { "unknown-file.txt" }, -1, -1 };
    }

    struct Fmt
    {
        std::ostringstream ss;

        template<typename T>
        Fmt& operator<<(const T& t)
        {
            ss << t;
            return *this;
        }

        operator std::string() const
        {
            return ss.str();
        }
    };

    template<typename TParent>
    class Definition
    {
        std::unordered_map<std::string, std::function<std::string(const TParent&)>> attributes;
        std::unordered_map<std::string, std::function<bool(const TParent&)>> bools;
        using ChildrenRet = std::pair<std::function<std::string (TParent)>, std::vector<Error>>;
        std::unordered_map<std::string, std::function<ChildrenRet (Node*)>> children;

        static std::string SyntaxError(const TParent&)
        {
            return "Syntax error";
        }

    public:
        Definition<TParent>& AddVar(std::string name, std::function<std::string (const TParent&)> getter)
        {
            attributes.insert({ name, getter });
            return *this;
        }

        Definition<TParent>& AddBool(std::string name, std::function<bool (const TParent&)> getter)
        {
            bools.insert({ name, getter });
            return *this;
        }

        template<typename TChild>
        Definition<TParent>& AddList(std::string name, std::function<std::vector<const TChild*>(const TParent&)> childSelector, Definition<TChild> childDef)
        {
            children.insert({ name, [=](Node* node)
            {
                auto [getter, errors] = childDef.Validate(node);
                if (errors.Length > 0) { return (SyntaxError, errors); }

                return { [=](const TParent& parent)
                    {
	                    const auto selected = childSelector(parent);
	                    std::string ret = "";
	                    for(const auto& c: selected)
	                    {
	                        ret += getter(c);
	                    }
	                    return ret;
                    }
                	, NoErrors()};
            }});
            return *this;
        }

        using GetterFunction = std::function<std::string(const TParent&)>;

        std::pair<GetterFunction, std::vector<Error>> Validate(Node* node)
        {
            if(auto* text = node->AsText())
            {
                const auto value = text->Value;
                return { [value](const TParent&) { return value; }, NoErrors()};
            }
            else if(auto* attribute = node->AsAttribute())
            {
                auto getter = attributes.find(attribute->Name);
                if (getter != attributes.end())
                {
                    return { SyntaxError, { Error {
                        attribute->Location,
                        Fmt{} << "Missing attribute " << attribute->Name << ": " << MatchStrings(attribute->Name, attributes.Keys)
					}}};
                }
                return { [getter](const TParent& parent) {return getter(parent); }, NoErrors() };
            }
            else if(auto* check = node->AsIf())
            {
                auto getter = bools.find(check->Name);
                if (getter == bools.end())
                {
                    return { SyntaxError, { Error {
                        check->Location,
                        Fmt{} << "Missing bool " << check->Name << ": " << MatchStrings(check->Name, bools.Keys)
                    }}};
                }

                auto [body, errors] = Validate(check->Body);
                if (errors.empty() == false) { return {SyntaxError, errors}; }

                return { [getter, body](const TParent& parent) { return getter(parent) ? body(parent) : ""; }, NoErrors };
            }
            else if(auto* iterate = node->AsIterate())
            {
                auto validator = children.find(iterate->Name);
                if (validator == children.end())
                {
                    return { SyntaxError, { Error{
                        iterate->Location,
                        Fmt{} << "Missing array " << iterate->Name << ": " << MatchStrings(iterate->Name, children.Keys
                    }}};
                }
                return validator(iterate->Body);
            }
            else if(auto* fc = node->AsFunctionCall())
            {
                auto [getter, errors] = Validate(fc->Arg);
                if (errors.empty() == false) { return { SyntaxError, errors }; }

                auto func = fc->Function;
                return ([getter, func](const TParent& parent) { return func(getter(parent)); }, NoErrors);
            }
            else if(auto* gr = node->AsGroup())
            {
                std::vector<GetterFunction> getters;
                std::vector<Error> errors;
                for(auto& n: gr->Nodes)
                {
                    auto [getter, local_errors] = Validate(n.get());
                    getters.emplace_back(std::move(getter));
                    errors.insert(errors.end(), local_errors.begin(), local_errors.end());
                }
                if (errors.empty() == false) { return { SyntaxError, errors }; }
                return { [getters](const TParent& parent)
                    {
	                    std::ostringstream ss;
	                    for(const auto& g: getters)
	                    {
	                        ss << g(parent);
	                    }
	                    return ss.str();
                    }
                , NoErrors};
            }
            else
            {
                assert(false);

                return { SyntaxError, { Error{
                        UnknownLocation(),
                        "internal error: unknown type"
                    }} };
            }
        }
    };

    internal static Task<(Func<T, std::string>, std::vector<Error>)> Parse<T>(std::string path, VfsRead vfs, std::unordered_map<std::string, FuncGenerator> functions, DirectoryInfo includeDir, Definition<T> definition)
    {
        auto source = vfs.ReadAllTextAsync(path);
        auto(tokens, lexerErrors) = Scanner(path, source);
        if (lexerErrors.Length > 0)
        {
            return (_ = > "Lexing failed", lexerErrors);
        }

        auto(node, parseErrors) = Parse(tokens, functions, includeDir, path.Extension, vfs);
        if (parseErrors.Length > 0)
        {
            return (_ = > "Parsing failed", parseErrors);
        }

        return definition.Validate(node);
    }

#if 0
    internal static std::unordered_map<std::string, FuncGenerator> DefaultFunctions()
    {
        auto culture = CultureInfo("en-US", false);

        auto t = std::unordered_map<std::string, FuncGenerator>();
        t.Add("capitalize", NoArguments(args = > Capitalize(args, true)));
        t.Add("lower", NoArguments(args = > culture.TextInfo.ToLower(args)));
        t.Add("upper", NoArguments(args = > culture.TextInfo.ToUpper(args)));
        t.Add("title", NoArguments(args = > culture.TextInfo.ToTitleCase(args)));

        static FuncGenerator NoArguments(Func<std::string, std::string> f)
        {
            return (location, args) = >
            {
                if (args.IsEmpty == false)
                {
                    return (
                        _ = > "syntax error",
                        std::vector.Create(
                            Error(location, "Expected zero arguments")));
                }
                return (arg = > f(arg), NoErrors);
            };
        }

        t.Add("rtrim", OptionalStringArgument((str, spaceChars) = > str.TrimEnd(spaceChars.ToCharArray()), " \t\n\r"));
        t.Add("ltrim", OptionalStringArgument((str, spaceChars) = > str.TrimStart(spaceChars.ToCharArray()), " \t\n\r"));
        t.Add("trim", OptionalStringArgument((str, spaceChars) = > str.Trim(spaceChars.ToCharArray()), " \t\n\r"));
        t.Add("zfill", OptionalIntArgument((str, count) = > str.PadLeft(count, '0'), 3));

        static FuncGenerator OptionalStringArgument(Func<std::string, std::string, std::string> f, std::string missing)
        {
            return (location, args) = >
            {
                return args.Length switch
                {
                    0 = > (arg = > f(arg, missing), NoErrors),
                        1 = > (arg = > f(arg, args[0].Argument), NoErrors),
                        _ = > (_ = > "syntax error",
                            std::vector.Create(Error(
                                location,
                                "Expected zero or one std::string argument")))
                };
            };
        }

        static FuncGenerator OptionalIntArgument(Func<std::string, int, std::string> f, int missing)
        {
            return (location, args) = >
            {
                return args.Length switch
                {
                    0 = > (arg = > f(arg, missing), NoErrors),
                        1 = > int.TryParse(args[0].Argument, out auto number)
                        ? (arg = > f(arg, number), NoErrors)
                        : (_ = > "syntax error",
                            std::vector.Create(
                                Error(location, "This function takes zero or one int argument"),
                                Error(args[0].Location, "this is not a int")
                            ))
                        ,
                        _ = > (_ = > "syntax error",
                            std::vector.Create(Error(
                                location,
                                "Expected zero or one int argument")))
                };
            };
        }

        t.Add("replace", StringStringArgument((arg, lhs, rhs) = > arg.Replace(lhs, rhs)));
        t.Add("substr", IntIntArgument((arg, lhs, rhs) = > arg.Substring(lhs, rhs)));
        return t;

        static (Func, std::vector<Error>) SyntaxError(params Error[] errors)
            = > (_ = > "syntax error", std::vector.Create(errors));

        static FuncGenerator StringStringArgument(Func<std::string, std::string, std::string, std::string> f)
        {
            return (location, args) = >
            {
                if (args.Length != 2)
                {
                    return SyntaxError(Error(location, "Expected two arguments"));
                }
                return (arg = > f(arg, args[0].Argument, args[1].Argument), NoErrors);
            };
        }

        static FuncGenerator IntIntArgument(Func<std::string, int, int, std::string> f)
        {
            return (location, args) = >
            {
                if (args.Length != 2)
                {
                    return SyntaxError(Error(location, "Expected two arguments"));
                }

                if (int.TryParse(args[0].Argument, out auto lhs) == false)
                {
                    return SyntaxError(Error(args[0].Location, "Not a integer"));
                }

                if (int.TryParse(args[1].Argument, out auto rhs) == false)
                {
                    return SyntaxError(Error(args[1].Location, "Not a integer"));
                }

                return (arg = > f(arg, lhs, rhs), NoErrors);
            };
        }

        static std::string Capitalize(std::string p, bool alsoFirstChar)
        {
            auto cap = alsoFirstChar;
            auto sb = StringBuilder();
            foreach(auto h in p.ToLower())
            {
                auto c = h;
                if (char.IsLetter(c) && cap)
                {
                    c = char.ToUpper(c);
                    cap = false;
                }
                if (char.IsWhiteSpace(c)) cap = true;
                sb.Append(c);
            }
            return sb.ToString();
        }
    }
#endif

    // --------------------------------------------------------------------------------------------

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
        ss << "Missing " << name << ", could be";
        for(const auto& c: candidates)
        {
            ss << c;
        }
        return ss.str();
#endif
    }


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

    struct Token
    {
        TokenType Type;
        std::string Lexeme;
        Location Location;
        std::string Value;
    };


    struct ScannerLocation
    {
        int Line;
        int Offset;
        int Index;
    };

    std::pair<std::vector<Token>, std::vector<Error>> Scanner(const std::string& file, const std::string& source)
    {
        auto start = ScannerLocation(1, 0, 0);
        auto current = start;
        auto insideCodeBlock = false;

        auto errors = std::vector<Error>();
        auto ret = std::vector<Token>();

        while (false == IsAtEnd())
        {
            start = current;
            ret.AddRange(ScanToken());
        }
        ret.Add(Token(TokenType.Eof, "", Location(file, current.Line, current.Offset), std::string.Empty));

        if (errors.Count != 0)
        {
            ret.Clear();
        }
        return (ret.ToImmutableArray(), errors.ToImmutableArray());

        void ReportError(Location loc, std::string message)
        {
            errors.Add(Error(loc, message));
        }

        std::vector<Token> ScanToken()
        {
            if (insideCodeBlock)
            {
                auto tok = ScanCodeToken();
                if (tok != null) yield return tok;
            }
            else
            {
                while (insideCodeBlock == false && false == IsAtEnd())
                {
                    auto beforeStart = current;
                    auto c = Advance();
                    if (c == '{' && Match('{'))
                    {
                        auto beginType = TokenType.BeginCode;
                        if (Peek() == '-')
                        {
                            Advance();
                            beginType = TokenType.BeginCodeTrim;
                        }
                        auto afterStart = current;
                        auto text = CreateToken(TokenType.Text, null, start, beforeStart);
                        if (text.Value.Length > 0)
                        {
                            yield return text;
                        }
                        insideCodeBlock = true;

                        yield return CreateToken(beginType, null, beforeStart, current);
                    }
                }

                if (IsAtEnd())
                {
                    auto text = CreateToken(TokenType.Text);
                    if (text.Value.Length > 0)
                    {
                        yield return text;
                    }
                }
            }
        }

        Token ? ScanCodeToken()
        {
            auto c = Advance();
            switch (c)
            {
            case '-':
                if (!Match('}'))
                {
                    ReportError(GetStartLocation(), "Detected rouge -");
                    return null;
                }

                if (!Match('}'))
                {
                    ReportError(GetStartLocation(), "Detected rouge -}");
                    return null;
                }

                insideCodeBlock = false;
                return CreateToken(TokenType.EndCodeTrim);
            case '}':
                if (!Match('}'))
                {
                    ReportError(GetStartLocation(), "Detected rouge {");
                    return null;
                }

                insideCodeBlock = false;
                return CreateToken(TokenType.EndCode);
            case '|': return CreateToken(TokenType.Pipe);
            case ',': return CreateToken(TokenType.Comma);
            case '(': return CreateToken(TokenType.LeftParen);
            case ')': return CreateToken(TokenType.RightParen);
            case '#': return CreateToken(TokenType.Hash);
            case '.': return CreateToken(TokenType.Dot);
            case '?': return CreateToken(TokenType.QuestionMark);

            case '/':
                if (!Match('*')) { return CreateToken(TokenType.Slash); }
                while (!(Peek() == '*' && PeekNext() == '/') && !IsAtEnd())
                {
                    Advance();
                }

                // skip * and /
                if (!IsAtEnd())
                {
                    Advance();
                    Advance();
                }

                return null;

            case '"':
                while (Peek() != '"' && !IsAtEnd())
                {
                    Advance();
                }

                if (IsAtEnd())
                {
                    ReportError(GetStartLocation(), "Unterminated std::string.");
                    return null;
                }

                // The closing ".
                Advance();

                // Trim the surrounding quotes.
                auto value = source.Substring(start.Index + 1, current.Index - start.Index - 2);
                return CreateToken(TokenType.Ident, value);

            case ' ':
            case '\r':
            case '\n':
            case '\t':
                return null;

            default:
                if (IsDigit(c))
                {
                    while (IsDigit(Peek())) Advance();

                    // Look for a fractional part.
                    if (Peek() == '.' && IsDigit(PeekNext()))
                    {
                        // Consume the "."
                        Advance();

                        while (IsDigit(Peek())) Advance();
                    }

                    return CreateToken(TokenType.Ident);
                }
                else if (IsAlpha(c))
                {
                    while (IsAlphaNumeric(Peek())) Advance();
                    auto ident = CreateToken(TokenType.Ident);

                    // check keywords
                    switch (ident.Value)
                    {
                    case "if": return ident with{ Type = TokenType.KeywordIf };
                    case "range": return ident with{ Type = TokenType.KeywordRange };
                    case "end": return ident with{ Type = TokenType.KeywordEnd };
                    case "include": return ident with{ Type = TokenType.KeywordInclude };
                    }

                    return ident;
                }
                else
                {
                    ReportError(GetStartLocation(), $"Unexpected character {c}");
                    return null;
                }
            }
        }

        static bool IsDigit(char c) = > c is >= '0' and <= '9';
        static bool IsAlpha(char c) = > c is >= 'a' and <= 'z' or >= 'A' and <= 'Z' or '_';
        static bool IsAlphaNumeric(char c) = > IsAlpha(c) || IsDigit(c);
        ScannerLocation NextChar() = > current with{ Index = current.Index + 1, Offset = current.Offset + 1 };

        bool Match(char expected)
        {
            if (IsAtEnd()) { return false; }
            if (source[current.Index] != expected) { return false; }

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
            return suggestedIndex >= source.Length ? '\0' : source[suggestedIndex];
        }

        char Advance()
        {
            auto ret = source[current.Index];
            current = NextChar();
            if (ret == '\n')
            {
                current = current with{ Offset = 0, Line = current.Line + 1 };
            }
            return ret;
        }

        Location GetStartLocation(ScannerLocation ? stt = null)
        {
            auto st = stt ? ? start;
            return Location(file, st.Line, st.Offset);
        }

        Token CreateToken(TokenType tt, std::string ? value = null, ScannerLocation ? begin = null, ScannerLocation ? end = null)
        {
            auto st = begin ? ? start;
            auto cu = end ? ? current;
            auto text = source.Substring(st.Index, cu.Index - st.Index);
            return Token(tt, text, GetStartLocation(st), value ? ? text);
        }

        bool IsAtEnd() = > current.Index >= source.Length;
    }


    private class ParseError : Exception { }
    private static std::pair<Node, std::vector<Error>> Parse(std::vector<Token> itok, std::unordered_map<std::string, FuncGenerator> functions, DirectoryInfo includeDir, std::string defaultExtension, VfsRead vfs)
    {
        static std::vector<Token> TrimTextTokens(std::vector<Token> tokens)
        {
            Token ? lastToken = null;
            foreach(auto tok in tokens)
            {
                switch (tok.Type)
                {
                case TokenType.BeginCodeTrim:
                    if (lastToken is{ Type: TokenType.Text })
                    {
                        yield return lastToken with{ Value = lastToken.Value.TrimEnd() };
                    }

                    lastToken = tok with{ Type = TokenType.BeginCode };
                    break;
                    case TokenType.Text when lastToken is{ Type: TokenType.EndCodeTrim }:
                        yield return lastToken with{ Type = TokenType.EndCode };
                        lastToken = tok with{ Value = tok.Value.TrimStart() };
                        break;
                    default:
                        if (lastToken != null)
                        {
                            yield return lastToken;
                        }

                        lastToken = tok;
                        break;
                }
            }

            if (lastToken != null)
            {
                yield return lastToken;
            }
        }

        static std::vector<Token> TrimEmptyStartEnd(std::vector<Token> tokens)
        {
            Token ? lastToken = null;
            foreach(auto tok in tokens)
            {
                if (lastToken is{ Type: TokenType.BeginCode } && tok.Type == TokenType.EndCode)
                {
                    lastToken = null;
                    continue;
                }

                if (lastToken != null)
                {
                    yield return lastToken;
                }

                lastToken = tok;
            }

            if (lastToken != null)
            {
                yield return lastToken;
            }
        }

        static std::vector<Token> TransformSingleCharsToKeywords(std::vector<Token> tokens)
        {
            auto eatIdent = false;
            Token ? lastToken = null;
            foreach(auto tok in tokens)
            {
                if (tok.Type == TokenType.Ident && eatIdent)
                {
                    eatIdent = false;
                    continue;
                }

                switch (tok.Type)
                {
                    case TokenType.Slash when lastToken is{ Type: TokenType.BeginCode }:
                        yield return lastToken;
                        lastToken = tok with{ Type = TokenType.KeywordEnd };
                        eatIdent = true;
                        break;
                        case TokenType.Hash when lastToken is{ Type:TokenType.BeginCode }:
                            yield return lastToken;
                            lastToken = tok with{ Type = TokenType.KeywordRange };
                            break;
                            case TokenType.QuestionMark when lastToken is{ Type: TokenType.BeginCode }:
                                yield return lastToken;
                                lastToken = tok with{ Type = TokenType.KeywordIf };
                                break;
                            default:
                                if (lastToken != null)
                                {
                                    yield return lastToken;
                                }

                                lastToken = tok;
                                break;
                }
            }

            if (lastToken != null)
            {
                yield return lastToken;
            }
        }

        auto tokens = TransformSingleCharsToKeywords(TrimEmptyStartEnd(TrimTextTokens(itok))).ToImmutableArray();

        auto current = 0;

        auto errors = std::vector<Error>();

        auto rootNode = ParseGroup();

        if (!IsAtEnd())
        {
            ReportError(Peek().Location, ExpectedMessage("EOF"));
        }

        if (errors.Count == 0)
        {
            return (rootNode, errors.ToImmutableArray());
        }
        return (Node.Text("Parsing failed", UnknownLocation), errors.ToImmutableArray());

        Task<Node> ParseGroup()
        {
            auto start = Peek().Location;
            auto nodes = std::vector<Node>();
            while (!IsAtEnd() && !(Peek().Type == TokenType.BeginCode && PeekNext() == TokenType.KeywordEnd))
            {
                try
                {
                    ParseNode(nodes);
                }
                catch (ParseError)
                {
                    Synchronize();
                }
            }

            return Node.Group(nodes, start);
        }

        ParseError ReportError(Location loc, std::string message)
        {
            errors.Add(Error(loc, message));
            return ParseError();
        }

        bool Match(params TokenType[] types)
        {
            if (!types.Any(Check)) return false;

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
            if (!IsAtEnd()) current++;
            return Previous();
        }

        bool IsAtEnd()
        {
            return Peek().Type == TokenType.Eof;
        }

        Token Peek()
        {
            return tokens[current];
        }

        TokenType PeekNext()
        {
            if (current + 1 >= tokens.Length) return TokenType.Eof;
            return tokens[current + 1].Type;
        }

        Token Previous()
        {
            return tokens[current - 1];
        }

        void Synchronize()
        {
            Advance();

            while (!IsAtEnd())
            {
                if (Previous().Type == TokenType.EndCode) return;

                switch (Peek().Type)
                {
                case TokenType.Text:
                    return;
                }

                Advance();
            }
        }

        static std::string TokenToMessage(Token token)
        {
            auto value = token.Type == TokenType.Text ? "" : $": {token.Lexeme}";
            return $"{token.Type}{value}";
        }

        Token Consume(TokenType type, std::string message)
        {
            if (Check(type)) return Advance();

            throw ReportError(Peek().Location, message);
        }

        FuncArgument ParseFunctionArg()
        {
            if (Peek().Type != TokenType.Ident)
            {
                throw ReportError(Peek().Location, ExpectedMessage("identifier"));
            }

            auto arg = Advance();
            return FuncArgument(arg.Location, arg.Value);
        }

        std::string ExtractAttributeName()
        {
            auto ident = Consume(TokenType.Ident, ExpectedMessage("IDENT"));
            return ident.Value;
        }

        std::string ExpectedMessage(std::string what)
        {
            return $"Expected {what} but found {TokenToMessage(Peek())}";
        }

        Task ParseNode(std::vector<Node> nodes)
        {
            switch (Peek().Type)
            {
            case TokenType.BeginCode:
                auto start = Peek().Location;
                Advance();

                if (Match(TokenType.KeywordRange))
                {
                    auto attribute = ExtractAttributeName();
                    Consume(TokenType.EndCode, ExpectedMessage("}}"));

                    auto group = ParseGroup();
                    Consume(TokenType.BeginCode, ExpectedMessage("{{"));
                    Consume(TokenType.KeywordEnd, ExpectedMessage("keyword end"));
                    Consume(TokenType.EndCode, ExpectedMessage("}}"));

                    nodes.Add(Node.Iterate(attribute, group, start));
                }
                else if (Match(TokenType.KeywordIf))
                {
                    auto attribute = ExtractAttributeName();
                    Consume(TokenType.EndCode, ExpectedMessage("}}"));

                    auto group = ParseGroup();
                    Consume(TokenType.BeginCode, ExpectedMessage("{{"));
                    Consume(TokenType.KeywordEnd, ExpectedMessage("keyword end"));
                    Consume(TokenType.EndCode, ExpectedMessage("}}"));

                    nodes.Add(Node.If(attribute, group, start));
                }
                else if (Match(TokenType.KeywordInclude))
                {
                    auto name = Consume(TokenType.Ident, ExpectedMessage("IDENT"));
                    auto includeLocation = Peek().Location;
                    Consume(TokenType.EndCode, ExpectedMessage("}}"));

                    auto firstFile = includeDir.GetFile(name.Value);
                    auto file = firstFile;
                    auto secondFile = firstFile;
                    if (vfs.Exists(file) == false)
                    {
                        secondFile = includeDir.GetFile(name.Value + defaultExtension);
                        file = secondFile;
                    }

                    if (vfs.Exists(file) == false)
                    {
                        ReportError(includeLocation, $"Unable to open file: tried {firstFile} and {secondFile}");
                    }
                    else
                    {
                        auto source = vfs.ReadAllTextAsync(file);
                        auto(scannerTokens, lexerErrors) = Scanner(file, source);
                        if (lexerErrors.Length > 0)
                        {
                            ReportError(includeLocation, "included from here...");
                            foreach(auto e in lexerErrors)
                            {
                                ReportError(e.Location, e.Message);
                            }
                            return;
                        }

                        auto(node, parseErrors) = Parse(scannerTokens, functions, includeDir, defaultExtension, vfs);
                        if (parseErrors.Length > 0)
                        {
                            ReportError(includeLocation, "included from here...");
                            foreach(auto e in parseErrors)
                            {
                                ReportError(e.Location, e.Message);
                            }

                            return;
                        }

                        nodes.Add(node);
                    }
                }
                else
                {
                    ParseAttributeToEnd(nodes);
                }
                break;
            case TokenType.Text:
                auto text = Advance();
                nodes.Add(Node.Text(text.Value, text.Location));
                break;
            default:
                throw ReportError(Peek().Location, $"Unexpected token {TokenToMessage(Peek())}");
            }
        }

        void ParseAttributeToEnd(std::vector<Node> nodes)
        {
            auto start = Peek().Location;
            Node node = Node.Attribute(ExtractAttributeName(), start);

            while (Peek().Type == TokenType.Pipe)
            {
                Advance();
                auto name = Consume(TokenType.Ident, ExpectedMessage("function name"));
                auto arguments = std::vector<FuncArgument>();

                if (Match(TokenType.LeftParen))
                {
                    while (Peek().Type != TokenType.RightParen && !IsAtEnd())
                    {
                        arguments.Add(ParseFunctionArg());

                        if (Peek().Type != TokenType.RightParen)
                        {
                            Consume(TokenType.Comma, ExpectedMessage("comma for the next function argument"));
                        }
                    }

                    Consume(TokenType.RightParen, ExpectedMessage(") to end function"));
                }

                if (functions.TryGetValue(name.Value, out auto funcGenerator))
                {
                    auto(func, funcParseErrors) = funcGenerator(name.Location, arguments.ToImmutableArray());
                    if (funcParseErrors.IsEmpty == false)
                    {
                        foreach(auto err in funcParseErrors)
                        {
                            ReportError(err.Location, err.Message);
                        }
                    }
                    node = Node.FunctionCall(name.Value, func, node, name.Location);
                }
                else
                {
                    ReportError(name.Location, $"Unknown function named {name.Value}: {MatchStrings(name.Value, functions.Keys)}");
                }
            }
            nodes.Add(node);

            Consume(TokenType.EndCode, ExpectedMessage("end token"));
        }
    }
}

