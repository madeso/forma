#include "catch2/catch_all.hpp"
#include "forma/template.hh"

#include <vector>
#include <string>

struct Song { std::string Artist;  std::string Title; std::string Album; int Track; };
forma::Definition<Song> MakeSongDef()
{
    return forma::Definition<Song>()
        .AddVar("artist", [](const Song& s) { return s.Artist; })
        .AddVar("title", [](const Song& s) { return s.Title; })
        .AddVar("album", [](const Song& s) { return s.Album; })
        .AddVar("track", [](const Song& s) { return std::to_string(s.Track); })
        ;
}

struct SongWithoutAlbum{ std::string Artist; std::string Title; bool HasStar; };
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

struct MixTape { std::vector<SongWithoutAlbum> Songs; };
forma::Definition<MixTape> MakeMixTapeDef()
{
    return forma::Definition<MixTape>()
        .AddList<SongWithoutAlbum>("songs", [](const MixTape& mt) { std::vector<const SongWithoutAlbum* > r;
    for (const auto& s : mt.Songs) r.emplace_back(&s);
    return r; }, MakeSongWithoutAlbumDef())
        ;
}

Song AbbaSong() { return { "ABBA", "dancing queen", "Arrival", 2 }; }
MixTape AwesomeMix() {
    return {
		{
	        SongWithoutAlbum{"Gloria Gaynor", "I Will Survive", true},
	        SongWithoutAlbum{"Nirvana", "Smells Like Teen Spirit", false}
        }
    };
}


TEST_CASE("Test1")
{
    auto file = cwd.GetFile("test.txt");
    read.AddContent(file, "{{artist}} - {{title}} ({{album}})");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeSongDef());
        evaluator(AbbaSong).Should().Be("ABBA - dancing queen (Arrival)");
        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test1Alt")
{
    auto file = cwd.GetFile("test.txt");
    // quoting attributes like strings should also work
    read.AddContent(file, "{{\"artist\"}} - {{\"title\"}} ({{\"album\"}})");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeSongDef());
        evaluator(AbbaSong).Should().Be("ABBA - dancing queen (Arrival)");
        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test1WithSpaces")
{
    auto file = cwd.GetFile("test.txt");
    // quoting attributes like strings should also work
    read.AddContent(file, "{{\"the artist\"}} - {{\"the title\"}} ({{\"the album\"}})");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeSongDefWithSpaces());
        evaluator(AbbaSong).Should().Be("ABBA - dancing queen (Arrival)");
        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test2")
{
    auto file = cwd.GetFile("test.txt");
    read.AddContent(file, "{{artist}} - {{title | title}} ( {{- album -}} )");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeSongDef());
        evaluator(AbbaSong).Should().Be("ABBA - Dancing Queen (Arrival)");
        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test3")
{
    auto file = cwd.GetFile("test.txt");
    read.AddContent(file, "{{track | zfill(3)}} {{- /** a comment **/ -}}  . {{title | title}}");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeSongDef());
        evaluator(AbbaSong).Should().Be("002. Dancing Queen");

        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test4")
{
    auto file = cwd.GetFile("test.txt");
    read.AddContent(file, "{{#songs}}[{{title}}]{{/songs}}");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeMixTapeDef());
        evaluator(AwesomeMix).Should().Be("[I Will Survive][Smells Like Teen Spirit]");

        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test4Readable")
{
    auto file = cwd.GetFile("test.txt");
    read.AddContent(file, "{{range songs}}[{{title}}]{{end}}");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeMixTapeDef());
        evaluator(AwesomeMix).Should().Be("[I Will Survive][Smells Like Teen Spirit]");

        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test5")
{
    auto file = cwd.GetFile("test.txt");
    read.AddContent(file, "{{range songs}} {{- include \"include.txt\" -}} {{end}}");
    read.AddContent(cwd.GetFile("include.txt"), "[{{title}}]");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeMixTapeDef());
        evaluator(AwesomeMix).Should().Be("[I Will Survive][Smells Like Teen Spirit]");

        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test5Alt")
{
    auto file = cwd.GetFile("test.txt");
    // use quotes to escape include keyword
    read.AddContent(file, "{{range songs}} {{- include \"include\" -}} {{end}}");
    read.AddContent(cwd.GetFile("include.txt"), "[{{title}}]");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeMixTapeDef());
        evaluator(AwesomeMix).Should().Be("[I Will Survive][Smells Like Teen Spirit]");

        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test6")
{
    auto file = cwd.GetFile("test.txt");
    read.AddContent(file, "{{range songs}} {{- include file -}} {{end}}");
    read.AddContent(cwd.GetFile("file.txt"), "[{{title}}]");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeMixTapeDef());
        evaluator(AwesomeMix).Should().Be("[I Will Survive][Smells Like Teen Spirit]");

        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}


TEST_CASE("Test7If")
{
    auto file = cwd.GetFile("test.txt");
    read.AddContent(file, "{{range songs -}} [ {{- if star -}} {{- title -}} {{- end -}} ] {{- end}}");
    using (new AssertionScope())
    {
        auto (evaluator, errors) = await forma::Parse(file, read, forma::DefaultFunctions(), cwd, MakeMixTapeDef());
        evaluator(AwesomeMix).Should().Be("[I Will Survive][]");

        errors.Should().BeEquivalentTo(new forma::Error[] { });
    }
}
