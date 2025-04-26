#include "template.hh"

#include <optional>
#include <string>
#include <functional>

namespace forma
{
FuncGeneratorResult SyntaxError(const std::vector<Error>& errors)
{
	return {[](const std::string&) { return "syntax error"; }, errors};
}

FuncGenerator NoArguments(std::function<std::string(const std::string&)> f)
{
	return [f](const Location& location, const std::vector<FuncArgument>& args)
	{
		if (args.empty() == false)
		{
			return SyntaxError({Error(location, "Expected zero arguments")});
		}
		return FuncGeneratorResult{[f](const std::string& arg) { return f(arg); }, NoErrors()};
	};
}

std::optional<int> parse_int(const std::string& s)
{
	try
	{
		return std::stoi(s);
	}
	catch (const std::invalid_argument& ex)
	{
		return std::nullopt;
	}
	catch (const std::out_of_range& ex)
	{
		return std::nullopt;
	}
}

FuncGenerator OptionalStringArgument(
	std::function<std::string(const std::string&, const std::string&)> f, std::string missing
)
{
	return [f, missing](const Location& location, const std::vector<FuncArgument>& args)
	{
		if (args.size() == 0)
		{
			return FuncGeneratorResult(
				[f, missing](const std::string& arg) { return f(arg, missing); }, NoErrors()
			);
		}
		else if (args.size() == 1)
		{
			const auto first_arg = args[0].Argument;
			return FuncGeneratorResult(
				[f, first_arg](const std::string& arg) { return f(arg, first_arg); }, NoErrors()
			);
		}
		else
		{
			return SyntaxError({Error(location, "Expected zero or one std::string argument")});
		}
	};
}

FuncGenerator OptionalIntArgument(std::function<std::string(std::string, int)> f, int missing)
{
	return [f, missing](const Location& location, const std::vector<FuncArgument>& args)
	{
		if (args.size() == 0)
		{
			return FuncGeneratorResult(
				[f, missing](const std::string& arg) { return f(arg, missing); }, NoErrors()
			);
		}
		else if (args.size() == 1)
		{
			const auto number = parse_int(args[0].Argument);
			if (number.has_value())
			{
				const auto the_number = *number;
				return FuncGeneratorResult(
					[f, the_number](const std::string& arg) { return f(arg, the_number); },
					NoErrors()
				);
			}
			else
			{
				return SyntaxError(
					{Error(location, "This function takes zero or one int argument"),
					 Error(args[0].Location, "this is not a int")}
				);
			}
		}
		else
		{
			return SyntaxError({Error(location, "Expected zero or one int argument")});
		}
	};
}

FuncGenerator StringStringArgument(
	std::function<std::string(std::string, std::string, std::string)> f
)
{
	return [f](const Location& location, const std::vector<FuncArgument>& args)
	{
		if (args.size() != 2)
		{
			return SyntaxError({Error(location, "Expected two arguments")});
		}
		return FuncGeneratorResult{
			[f, args](const std::string& arg) -> std::string
			{ return f(arg, args[0].Argument, args[1].Argument); },
			NoErrors()
		};
	};
}

FuncGenerator IntIntArgument(std::function<std::string(std::string, int, int)> f)
{
	return [f](const Location& location, const std::vector<FuncArgument>& args)
	{
		if (args.size() != 2)
		{
			return SyntaxError({Error(location, "Expected two arguments")});
		}

		const auto lhs = parse_int(args[0].Argument);
		if (lhs.has_value() == false)
		{
			return SyntaxError({Error(args[0].Location, "Not a integer")});
		}

		const auto rhs = parse_int(args[1].Argument);
		if (rhs.has_value() == false)
		{
			return SyntaxError({Error(args[1].Location, "Not a integer")});
		}

		return FuncGeneratorResult{
			[f, lhs, rhs](const std::string& arg) -> std::string { return f(arg, *lhs, *rhs); },
			NoErrors()
		};
	};
}

std::unordered_map<std::string, FuncGenerator> DefaultFunctions()
{
	// auto culture = CultureInfo("en-US", false);

	auto t = std::unordered_map<std::string, FuncGenerator>();
	t.insert(
		{"capitalize",
		 NoArguments([](const std::string& args) { return strings::Capitalize(args, true); })}
	);
	t.insert({"lower", NoArguments([](const std::string& args) { return strings::ToLower(args); })}
	);
	t.insert({"upper", NoArguments([](const std::string& args) { return strings::ToUpper(args); })}
	);
	t.insert(
		{"title", NoArguments([](const std::string& args) { return strings::ToTitleCase(args); })}
	);

	t.insert(
		{"rtrim",
		 OptionalStringArgument(
			 [](const std::string& str, const std::string& spaceChars)
			 { return strings::TrimEnd(str, spaceChars); },
			 strings::default_space()
		 )}
	);
	t.insert(
		{"ltrim",
		 OptionalStringArgument(
			 [](const std::string& str, const std::string& spaceChars)
			 { return strings::TrimStart(str, spaceChars); },
			 strings::default_space()
		 )}
	);
	t.insert(
		{"trim",
		 OptionalStringArgument(
			 [](const std::string& str, const std::string& spaceChars)
			 { return strings::Trim(str, spaceChars); },
			 strings::default_space()
		 )}
	);
	t.insert(
		{"zfill",
		 OptionalIntArgument(
			 [](const std::string& str, int count) { return strings::PadLeft(str, count, '0'); }, 3
		 )}
	);

	t.insert(
		{"replace",
		 StringStringArgument(
			 [](const std::string& arg, const std::string& lhs, const std::string& rhs)
			 { return strings::Replace(arg, lhs, rhs); }
		 )}
	);
	t.insert(
		{"substr",
		 IntIntArgument([](const std::string& arg, int lhs, int rhs)
						{ return strings::Substring(arg, lhs, rhs); })}
	);
	return t;
}
}  //  namespace forma
