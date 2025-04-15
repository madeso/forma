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
#include <unordered_map>
#include <cassert>
#include <sstream>

#include "forma/core.hh"
#include "forma/scanner.hh"
#include "forma/parser.hh"

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
	template<typename TParent>
    class Definition
    {
        std::unordered_map<std::string, std::function<std::string(const TParent&)>> attributes;
        std::unordered_map<std::string, std::function<bool(const TParent&)>> bools;
        using ChildrenRet = std::pair<std::function<std::string (TParent)>, std::vector<Error>>;
        using ChildMapFunction = std::function<ChildrenRet(std::shared_ptr<Node>)>;
        std::unordered_map<std::string, ChildMapFunction> children;

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
            children.insert({ name, [=](std::shared_ptr<Node> node) ->ChildrenRet
            {
                auto [getter, errors] = childDef.Validate(node);
                if (errors.size() > 0) { return { SyntaxError, errors }; }

                return { [=](const TParent& parent)
                    {
	                    const auto selected = childSelector(parent);
	                    std::string ret;
	                    for(const TChild* c: selected)
	                    {
                            const std::string& val = getter(*c);
	                        ret += val;
	                    }
	                    return ret;
                    }
                	, NoErrors()};
            }});
            return *this;
        }

        using GetterFunction = std::function<std::string(const TParent&)>;
        using ValidationResult = std::pair<GetterFunction, std::vector<Error>>;

        ValidationResult Validate(std::shared_ptr<Node> node) const
        {
            if(auto* text = node->AsText())
            {
                const auto value = text->Value;
                return { [value](const TParent&) { return value; }, NoErrors()};
            }
            else if(auto* attribute = node->AsAttribute())
            {
                const auto getter = attributes.find(attribute->Name);
                if (getter == attributes.end())
                {
                    return { SyntaxError, { Error {
                        attribute->Location,
                        Fmt{} << "Missing attribute " << attribute->Name << ": " << MatchStrings(attribute->Name, KeysOf(attributes))
					}}};
                }
                const auto& getter_func = getter->second;
                return {
                    [getter_func](const TParent& parent) {return getter_func(parent); }
                	, NoErrors()
                };
            }
            else if(auto* check = node->AsIf())
            {
                const auto getter = bools.find(check->Name);
                if (getter == bools.end())
                {
                    return { SyntaxError, { Error {
                        check->Location,
                        Fmt{} << "Missing bool " << check->Name << ": " << MatchStrings(check->Name, KeysOf(bools))
                    }}};
                }

                const auto [body, errors] = Validate(check->Body);
                if (errors.empty() == false) { return {SyntaxError, errors}; }

                const auto getter_func = getter->second;
                return { [getter_func, body](const TParent& parent) -> std::string { return getter_func(parent) ? body(parent) : ""; }, NoErrors()};
            }
            else if(auto* iterate = node->AsIterate())
            {
                auto validator = children.find(iterate->Name);
                if (validator == children.end())
                {
                    return { SyntaxError, { Error{
                        iterate->Location,
                        Fmt{} << "Missing array " << iterate->Name << ": " << MatchStrings(iterate->Name, KeysOf(children))
                    }}};
                }
                return validator->second(iterate->Body);
            }
            else if (auto* fc = node->AsFunctionCall())
            {
                auto [getter, errors] = Validate(fc->Arg);
                if (errors.empty() == false) { return { SyntaxError, errors }; }

                auto func = fc->Function;
                return {[getter, func](const TParent& parent) { return func(getter(parent)); }, NoErrors()};
            }
            else if(auto* gr = node->AsGroup())
            {
                std::vector<GetterFunction> getters;
                std::vector<Error> errors;
                for(auto& n: gr->Nodes)
                {
                    auto [getter, local_errors] = Validate(n);
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
                , NoErrors()};
            }
            else
            {
                assert(false);

                return { SyntaxError, { Error{
                        UnknownLocation(),
                        "error: unknown type"
                    }} };
            }
        }
    };

    template<typename T>
    using BuildResult = std::pair<std::function<std::string(const T&)>, std::vector<Error>>;

    template<typename T>
    BuildResult<T> Build(std::string path, VfsRead* vfs, std::unordered_map<std::string, FuncGenerator> functions, DirectoryInfo* includeDir, Definition<T> definition)
    {
        auto source = vfs->ReadAllText(path);
        auto [tokens, lexerErrors] = Scan(path, source);
        if (lexerErrors.size() > 0)
        {
            return BuildResult<T>{[](const T&) { return "Lexing failed"; }, lexerErrors};
        }

        auto [node, parseErrors] = forma::Parse(tokens, functions, includeDir, vfs->GetExtension(path), vfs);
        if (parseErrors.size() > 0)
        {
            return BuildResult<T>{ [](const T&) { return "Parsing failed"; }, parseErrors };
        }

        return definition.Validate(node);
    }

    std::unordered_map<std::string, FuncGenerator> DefaultFunctions();

}

