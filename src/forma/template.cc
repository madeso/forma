#include "template.hh"

namespace forma
{
#if 1
    std::unordered_map<std::string, FuncGenerator> DefaultFunctions()
    {
        return {};
    }
#else
    static std::unordered_map<std::string, FuncGenerator> DefaultFunctions()
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
}
 