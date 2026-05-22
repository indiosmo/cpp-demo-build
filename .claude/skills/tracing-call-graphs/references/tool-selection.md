# Tool Selection For Call Graph Tracing

Use semantic tools first, then search as a fallback.

## LSP

### Go To Definition

Use for concrete free functions, non-template methods, and symbols in known
files.

### Hover

Use to inspect member, variable, and expression types before tracing a call.

### Outgoing Calls

Use as a first pass for direct calls. Fill gaps manually for templates,
variant visitors, callback fields, and macro-wrapped result propagation.

### Find Implementations

Use for virtual methods and abstract interfaces. Review all implementations,
then select the reachable one from the runtime composition.

### Find References

Use for incoming calls and composition sites. It can miss template
instantiations, so confirm with search when the type is templated.

## Search

Prefer `rg`.

Useful patterns:

```bash
rg -n "make_leaf_error|make_error|new_error|throw" src submission/src
rg -n "LAB_ASSIGN|LAB_CHECK|KRAKEN_LEAF_CHECK|BOOST_LEAF_ASSIGN" src submission/src
rg -n "using request = std::variant|std::variant<" src submission/src
rg -n "on_[a-z_]+ =" src submission/src
```

Use search when:

- tracing macro names;
- locating error struct definitions;
- finding template instantiations;
- finding callback assignment sites;
- LSP cannot resolve a symbol.

## Read

Read full files when control flow matters. Call graphs are often wrong when
assembled only from definitions without the surrounding member fields,
callbacks, and error boundary.

## Decision Flow

```text
Need to trace a call site?
  |
  +-- Know the concrete type?
  |     yes -> go to definition
  |     no  -> read owning class header
  |              |
  |              +-- abstract? -> find implementations
  |              +-- template? -> find instantiation
  |              +-- variant? -> read alternatives
  |              +-- callback? -> read composition site
  |
  +-- At callee:
        |
        +-- returns result? -> recurse
        +-- creates error or throws? -> record
        +-- plain value / void? -> stop
        +-- external opaque? -> note and stop
```
