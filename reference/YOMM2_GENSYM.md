<sub>/ [home](/README.md) / [reference](README.md) </sub>

## YOMM2_GENSYM()
<sub>defined in header <yorel/yomm2/symbols.hpp>
<yorel/yomm2/keywords.hpp></sub>

---
Macro `YOMM2_GENSYM` expands to a new C++ identifier each time it is called. The
symbol is based on the `__COUNTER__` preprocessor macro, and decorated in such a
way that it is unlikely to clash with user-defined symbols.

`YOMM2_GENSYM` provides a convenient way of allocating [static
objects](static-object.md) for registering classes, methods, or definitions.

### example

```c++
int YOMM2_GENSYM;
int YOMM2_GENSYM; // not a redefinition, this is a new symbol
```
