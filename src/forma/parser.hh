#pragma once

#include <memory>
#include <stdexcept>

#include "forma/core.hh"
#include "forma/scanner.hh"

namespace forma
{
namespace node
{
	struct Text;
	struct Attribute;
	struct Iterate;
	struct If;
	struct FunctionCall;
	struct Group;
}  //  namespace node

struct Node
{
	virtual ~Node() = default;

	virtual node::Text* AsText()
	{
		return nullptr;
	}

	virtual node::Attribute* AsAttribute()
	{
		return nullptr;
	}

	virtual node::Iterate* AsIterate()
	{
		return nullptr;
	}

	virtual node::If* AsIf()
	{
		return nullptr;
	}

	virtual node::FunctionCall* AsFunctionCall()
	{
		return nullptr;
	}

	virtual node::Group* AsGroup()
	{
		return nullptr;
	}
};

namespace node
{
	struct Text : Node
	{
		Text(std::string v, forma::Location l);
		std::string Value;
		forma::Location Location;

		Text* AsText() override;
	};

	struct Attribute : Node
	{
		Attribute(std::string Name, forma::Location Location);

		std::string Name;
		forma::Location Location;

		Attribute* AsAttribute() override;
	};

	struct Iterate : Node
	{
		Iterate(std::string n, std::shared_ptr<Node> b, forma::Location l);

		std::string Name;
		std::shared_ptr<Node> Body;
		forma::Location Location;

		Iterate* AsIterate() override;
	};

	struct If : Node
	{
		If(std::string n, std::shared_ptr<Node> b, forma::Location l);
		std::string Name;
		std::shared_ptr<Node> Body;
		forma::Location Location;

		If* AsIf() override;
	};

	struct FunctionCall : Node
	{
		FunctionCall(std::string n, Func f, std::shared_ptr<Node> a, forma::Location l);

		std::string Name;
		Func Function;
		std::shared_ptr<Node> Arg;
		forma::Location Location;

		FunctionCall* AsFunctionCall() override;
	};

	struct Group : Node
	{
		Group(std::vector<std::shared_ptr<Node>> n, forma::Location l);

		std::vector<std::shared_ptr<Node>> Nodes;
		forma::Location Location;

		Group* AsGroup() override;
	};
}  //  namespace node

using ParseResult = std::pair<std::shared_ptr<Node>, std::vector<Error>>;
ParseResult Parse(
	std::vector<Token> itok,
	std::unordered_map<std::string, FuncGenerator> functions,
	DirectoryInfo* includeDir,
	std::string defaultExtension,
	VfsRead* vfs
);

}  //  namespace forma
