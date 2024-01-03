# Forma

Forma, swedish, to shape, to form to template. A simple (typesafe) templating engine ported from my c# template engine.

Work in progress


## API:
```cpp
const auto [generator, error] = forma::Parse(...);
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

