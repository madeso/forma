# Forma

Forma, swedish, to shape, to form to template. A simple (typesafe) templating engine ported from my c# template engine.

Work in progress


## API:
```cpp
struct MyClass;

const auto [generator, error] = forma::Parse(
        // integrate with your own file system
        file, &vfs, &cwd,
        // custom functions to transform the data
        forma::DefaultFunctions(),
        // define what and how your class is exposed
        forma::Definition<MyClass>()
            .AddVar("foo", [](const MyClass& s) { return s.foo_bar; })
            // .AddBool(...)
            // .AddList(...)
    );
// either you get
//  - a default dummy generator with errors or
//  - the parsed generator with no errors
// either way, there is no more parsing you have a generator:
//  std::function<std::string(const MyClass&)>

MyClass myClass = ...;
std::string ret = generator(myClass);
```

## Template syntax:
```
{{ prop }} {{- "also prop, trim printable spaces" -}}
{{prop | function | function(with_arguments)}}
{{include file}} {{include "file/with.extension"}}
{{#list}}repeated{{/list}} {{range also_list}}repeated{{end}}
{{if bool_prop}}perhaps{{end}}
```

