#pragma once

#include <string>
#include <functional>
#include <sstream>

namespace forma
{
    // ------------------------------------------------------------------------
    // file integration
    struct VfsRead
    {
        virtual ~VfsRead() = default;
        virtual std::string ReadAllText(const std::string& path) = 0;
        virtual bool Exists(const std::string& f) = 0;
        virtual std::string GetExtension(const std::string& file_path) = 0;
    };
    struct DirectoryInfo
    {
        virtual ~DirectoryInfo() = default;
        virtual std::string GetFile(const std::string& nameAndExtension) = 0;
    };


    // ------------------------------------------------------------------------
    // forma util

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


    // ------------------------------------------------------------------------
    // common forma

    struct Location
    {
        std::string File;
        int Line;
        int Offset;

        auto operator<=>(const Location& rhs) const = default;
    };

    template<typename S>
    S& operator<<(S& s, const Location& loc)
    {
        return s << loc.File << ':' << loc.Line << ':' << loc.Offset;
    }

    struct Error
    {
        forma::Location Location;
        std::string Message;

        auto operator<=>(const Error& rhs) const = default;
    };

    template<typename S>
    S& operator<<(S& s, const Error& err)
    {
        return s << err.Location << ": " << err.Message;
    }


    struct FuncArgument
    {
        forma::Location Location;
        std::string Argument;
    };

    // apply dynamic std::string function and return result
    using Func = std::function<std::string(std::string)>;

    // parse arguments, return function call or error
    using FuncGeneratorResult = std::pair<Func, std::vector<Error>>;
    using FuncGenerator = std::function<FuncGeneratorResult(Location call, std::vector<FuncArgument> arguments)>;

    std::vector<Error> NoErrors();
    Location UnknownLocation();

    template<typename K, typename V>
    std::vector<K> KeysOf(const std::unordered_map<K, V>& m)
    {
        std::vector<K> ks;
        for (const auto& pair : m)
        {
            ks.push_back(pair.first);
        }
        return ks;
    }

    std::string MatchStrings(std::string& name, const std::vector<std::string>& candidates);

    namespace strings
    {
        std::string default_space();
        std::string TrimStart(const std::string& s, const std::string& = default_space());
        std::string TrimEnd(const std::string& s, const std::string& = default_space());
        std::string Trim(const std::string& s, const std::string & = default_space());

        std::string Capitalize(const std::string& p, bool alsoFirstChar);

        std::string ToLower(const std::string& args);
        std::string ToUpper(const std::string& args);
        std::string ToTitleCase(const std::string& args);

        std::string PadLeft(const std::string& s, int count, char c);
        std::string Replace(const std::string& arg, const std::string& lhs, const std::string& rhs);
        std::string Substring(const std::string& arg, int lhs, int rhs);
    }
}
