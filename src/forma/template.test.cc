#include "catch2/catch_all.hpp"
#include "catch2/matchers/catch_matchers_vector.hpp"

#include "forma/template.hh"

#include <vector>
#include <string>
#include <unordered_map>

struct VfsReadTest : forma::VfsRead
{
	std::unordered_map<std::string, std::string> contents;

	std::string ReadAllText(const std::string& path) override
	{
		const auto found = contents.find(path);
		if (found == contents.end()) return "failed to find file";

		const auto ret = found->second;
		contents.erase(found);
		return ret;
	}

	bool Exists(const std::string& path) override
	{
		const auto found = contents.find(path);
		return found != contents.end();
	}

	void AddContent(const std::string& name, const std::string& content)
	{
		contents.insert({ name, content });
	}

	std::string GetExtension(const std::string& file_path) override
	{
		auto pos = file_path.find_last_of('.');
		if (pos == std::string::npos) return ""; // No extension found
		return file_path.substr(pos + 1);
	}
};

struct DirectoryInfoTest : forma::DirectoryInfo
{
	std::string dir;
	explicit DirectoryInfoTest(const std::string& d) : dir(d) {}

	std::string GetFile(const std::string& nameAndExtension) override
	{
		return dir + nameAndExtension;
	}
};

struct Song
{
	std::string Artist;
	std::string Title;
	std::string Album;
	int Track;
};

struct SongWithoutAlbum
{
	std::string Artist;
	std::string Title;
	bool HasStar;
};


struct MixTape
{
	std::vector<SongWithoutAlbum> Songs;
};

Song AbbaSong()
{
	return { "ABBA", "dancing queen", "Arrival", 2 };
}

MixTape AwesomeMix() {
	return {
		{
			SongWithoutAlbum{"Gloria Gaynor", "I Will Survive", true},
			SongWithoutAlbum{"Nirvana", "Smells Like Teen Spirit", false}
		}
	};
}

forma::Definition<Song> MakeSongDef()
{
	return forma::Definition<Song>()
		.AddVar("artist", [](const Song& s) { return s.Artist; })
		.AddVar("title", [](const Song& s) { return s.Title; })
		.AddVar("album", [](const Song& s) { return s.Album; })
		.AddVar("track", [](const Song& s) { return std::to_string(s.Track); })
		;
}

forma::Definition<SongWithoutAlbum> MakeSongWithoutAlbumDef() {
	return forma::Definition<SongWithoutAlbum>()
		.AddVar("artist", [](const SongWithoutAlbum& s) { return s.Artist; })
		.AddVar("title", [](const SongWithoutAlbum& s) { return s.Title; })
		.AddVar("album", [](const SongWithoutAlbum& s) -> std::string { throw "Song doesn't have a album :("; }) // this shouldn't be called
		.AddBool("star", [](const SongWithoutAlbum& s) { return s.HasStar; })
		;
}

forma::Definition<Song> MakeSongDefWithSpaces() {
	return forma::Definition<Song>()
		.AddVar("the artist", [](const Song& s) { return s.Artist; })
		.AddVar("the title", [](const Song& s) { return s.Title; })
		.AddVar("the album", [](const Song& s) { return s.Album; })
		;
}


forma::Definition<MixTape> MakeMixTapeDef()
{
	return forma::Definition<MixTape>()
		.AddList<SongWithoutAlbum>("songs", [](const MixTape& mt) { std::vector<const SongWithoutAlbum* > r;
	for (const auto& s : mt.Songs) r.emplace_back(&s);
	return r; }, MakeSongWithoutAlbumDef())
		;
}


TEST_CASE("all")
{
	DirectoryInfoTest cwd("C:\\");
	VfsReadTest read;

#define NO_ERRORS(errors) CHECK_THAT(errors, Catch::Matchers::Equals(std::vector<forma::Error>{}))

	SECTION("Test1")
	{
		auto file = cwd.GetFile("test.txt");
		read.AddContent(file, "{{artist}} - {{title}} ({{album}})");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeSongDef());

		CHECK(evaluator(AbbaSong()) == "ABBA - dancing queen (Arrival)");
		NO_ERRORS(errors);
	}


	SECTION("Test1Alt")
	{
		auto file = cwd.GetFile("test.txt");
		// quoting attributes like strings should also work
		read.AddContent(file, "{{\"artist\"}} - {{\"title\"}} ({{\"album\"}})");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeSongDef());

		CHECK(evaluator(AbbaSong()) == "ABBA - dancing queen (Arrival)");
		NO_ERRORS(errors);
	}


	SECTION("Test1WithSpaces")
	{
		auto file = cwd.GetFile("test.txt");
		// quoting attributes like strings should also work
		read.AddContent(file, "{{\"the artist\"}} - {{\"the title\"}} ({{\"the album\"}})");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeSongDefWithSpaces());

		CHECK(evaluator(AbbaSong()) == "ABBA - dancing queen (Arrival)");
		NO_ERRORS(errors);
	}


	SECTION("Test2")
	{
		auto file = cwd.GetFile("test.txt");
		read.AddContent(file, "{{artist}} - {{title | title}} ( {{- album -}} )");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeSongDef());

		CHECK(evaluator(AbbaSong()) == "ABBA - Dancing Queen (Arrival)");
		NO_ERRORS(errors);
	}


	SECTION("Test3")
	{
		auto file = cwd.GetFile("test.txt");
		read.AddContent(file, "{{track | zfill(3)}} {{- /** a comment **/ -}}  . {{title | title}}");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeSongDef());

		CHECK(evaluator(AbbaSong()) == "002. Dancing Queen");
		NO_ERRORS(errors);
	}


	SECTION("Test4")
	{
		auto file = cwd.GetFile("test.txt");
		read.AddContent(file, "{{#songs}}[{{title}}]{{/songs}}");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeMixTapeDef());

		CHECK(evaluator(AwesomeMix()) == "[I Will Survive][Smells Like Teen Spirit]");
		NO_ERRORS(errors);
	}


	SECTION("Test4Readable")
	{
		auto file = cwd.GetFile("test.txt");
		read.AddContent(file, "{{range songs}}[{{title}}]{{end}}");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeMixTapeDef());

		CHECK(evaluator(AwesomeMix()) == "[I Will Survive][Smells Like Teen Spirit]");
		NO_ERRORS(errors);
	}


	SECTION("Test5")
	{
		auto file = cwd.GetFile("test.txt");
		read.AddContent(file, "{{range songs}} {{- include \"include.txt\" -}} {{end}}");
		read.AddContent(cwd.GetFile("include.txt"), "[{{title}}]");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeMixTapeDef());

		CHECK(evaluator(AwesomeMix()) == "[I Will Survive][Smells Like Teen Spirit]");
		NO_ERRORS(errors);
	}


	SECTION("Test5Alt")
	{
		auto file = cwd.GetFile("test.txt");
		// use quotes to escape include keyword
		read.AddContent(file, "{{range songs}} {{- include \"include\" -}} {{end}}");
		read.AddContent(cwd.GetFile("include.txt"), "[{{title}}]");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeMixTapeDef());

		CHECK(evaluator(AwesomeMix()) == "[I Will Survive][Smells Like Teen Spirit]");
		NO_ERRORS(errors);
	}


	SECTION("Test6")
	{
		auto file = cwd.GetFile("test.txt");
		read.AddContent(file, "{{range songs}} {{- include file -}} {{end}}");
		read.AddContent(cwd.GetFile("file.txt"), "[{{title}}]");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeMixTapeDef());

		CHECK(evaluator(AwesomeMix()) == "[I Will Survive][Smells Like Teen Spirit]");
		NO_ERRORS(errors);
	}


	SECTION("Test7If")
	{
		auto file = cwd.GetFile("test.txt");
		read.AddContent(file, "{{range songs -}} [ {{- if star -}} {{- title -}} {{- end -}} ] {{- end}}");

		auto [evaluator, errors] = forma::Build(file, &read, forma::DefaultFunctions(), &cwd, MakeMixTapeDef());

		CHECK(evaluator(AwesomeMix()) == "[I Will Survive][]");
		NO_ERRORS(errors);
	}

}