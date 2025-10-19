# Name Resolution with `typedef` and `using`

## Table of Contents

- Introduction
- Name Resolution Rules
- How to Use `typedef` and `using` (Differences)
- Examples
  - Example 1: `typedef` alias for a simple type
  - Example 2: `using` alias for a template
  - Example 3: Name conflict with a `using`-declaration
  - Example 4: Ambiguity with `using namespace`
  - Example 5: Template alias using `using`
  - Example 6: Incorrect two-phase lookup usage
  - Example 7: Correct qualifier usage in templates
  - Example 8: `typedef` vs `using` for a complex type
  - Example 9: `using`-declaration for a type
  - Example 10: Implicit conflicts with `using namespace`
- Summary
- Our Experiences

---

## 1. Introduction

- **Type aliases** are synonyms for existing types. They improve readability by giving meaningful names to types (especially complex ones).

- Aliases can be declared in **global, block, class, or namespace** scope. This means you can have aliases inside functions, inside classes as member typedefs, or at namespace/global scope.

- In modern C++, `using` (since C++11) is preferred. It can be used exactly like `typedef` for simple aliases and also to create _alias templates_ for generic types.


---

## 2. Name Resolution Rules

- **Uniform lookup rules:** The compiler treats alias names just like any other name. In other words, aliases obey the same C++ scope and hiding rules as class or namespace names.

- **Scope and hiding:** An alias’s visibility depends on where it’s declared. A local declaration can hide an alias from an outer scope. For example, if a typedef `X` exists globally, a local variable `X` inside a function will shadow that alias. To refer to an alias from an outer scope, you must qualify it (e.g. `Outer::AliasName`).

- **Dependent names in templates:** When using a type alias inside a template (as a dependent name), you still use `typename` as needed, just as with any nested type.

- **Argument-dependent lookup (ADL):** When calling functions, C++ considers the _underlying types_ of arguments, not alias names. **Typedefs/`using` aliases do not contribute to ADL**. 

---

## 3. How to Use `typedef` and `using` (Differences)

- **Equivalence (for simple aliases):** For basic aliases, `typedef` and `using` are equivalent. Example: `typedef int T1;` and `using T2 = int;` both make a synonym for `int`. The choice is syntactic.

- **Readability:** `using` syntax is often clearer to read (aliases appear left-to-right). With `typedef`, the alias name is at the end of the declaration, which can be confusing for complex types.

- **Templates:** The key advantage of `using` is alias templates. You _cannot_ create a templated alias with `typedef`; `using` can declare alias templates (e.g. `template<class T> using Vec = std::vector<T>;`).

---

## 4. Examples

Below are 10 slides with code examples.

### Example 1: `typedef` alias for a simple type

<summary>example1.cpp</summary>

```cpp
#include <iostream>

// Example 1: typedef alias for a simple type
typedef int myint;

int main() {
    myint x = 5;
    std::cout << "myint x = " << x << "\n";
    return 0;
}
```



**Explanation:**
`typedef int myint;` creates a new name `myint` that’s exactly equivalent to `int`. In `main()`, `myint x = 5;` declares an integer variable. This always compiles because `typedef` does not introduce a new type, just an alias.

---

### Example 2: `using` alias for a template

<summary>example2.cpp</summary>

```cpp
#include <iostream>
#include <vector>

// Example 2: using alias for a template
using myvec = std::vector<int>;

int main() {
    myvec v = {1, 2, 3};
    std::cout << "myvec size = " << v.size() << "\n";
    return 0;
}
```


**Explanation:**
`using myvec = std::vector<int>;` defines an alias `myvec` for `std::vector<int>`. It’s more readable than a `typedef` with templates. The code compiles and prints the vector’s size because `myvec` behaves exactly like the instantiated template.

---

### Example 3: Name conflict with a `using`-declaration

<summary>example3.cpp</summary>

```cpp
#include <iostream>

namespace A { int x; }
using A::x; // brings A::x into the global scope

int main() {
    int x = 1;
    std::cout << x; // error: conflict with A::x
    return 0;
}
```


**Explanation:**
`using A::x;` imports `A::x` into the global namespace. Declaring your own `x` causes ambiguity between `A::x` and your `x`, leading to a compile-time error.

---

### Example 4: Ambiguity with `using namespace`

<summary>example4.cpp</summary>

```cpp
#include <iostream>

namespace B { void f(); }
using namespace B;

void f(int) { std::cout << "f(int)\n"; }

int main() {
    f(); // error: ambiguous between B::f and ::f(int)
    return 0;
}
```


**Explanation:**
`using namespace B;` brings all of `B` into the global scope. Since `B` has `void f()` and you define `void f(int)`, the call `f()` is ambiguous.

---

### Example 5: Template alias using `using`

<summary>example5.cpp</summary>

```cpp
#include <iostream>

// Example 5: template alias via using
template<typename T>
using Ptr = T*;

int main() {
    Ptr<int> p = new int(42);
    std::cout << "Ptr<int> p = " << *p << "\n";
    delete p;
    return 0;
}
```


**Explanation:**
`using Ptr = T*;` creates a parameterized alias; `Ptr<int>` is equivalent to `int*`. This cannot be done with `typedef`.

---

### Example 6: Incorrect two-phase lookup usage

<summary>example6.cpp</summary>

```cpp
#include <iostream>

namespace M {
    template<typename T>
    struct S { static void g() { std::cout << "M::S<>::g()"; } };
}

template<typename U>
void h_wrong() {
    S<U>::g(); // error: two-phase lookup doesn't see M::S in phase one
}

int main() {
    return 0;
}
```


**Explanation:**
Unqualified `S<U>::g()` in a dependent context fails two-phase lookup because `S` isn’t found during the first phase.

---

### Example 7: Correct qualifier usage in templates

<summary>example7.cpp</summary>

```cpp
#include <iostream>

namespace M {
    template<typename T>
    struct S { static void g() { std::cout << "M::S<>::g()"; } };
}

template<typename U>
void h_correct() {
    M::template S<U>::g();
}

int main() {
    std::cout << "Calling corrected h_correct: ";
    h_correct<int>();
    std::cout << "\n";
    return 0;
}
```


**Explanation:**
`M::template S<U>::g()` tells the compiler that `S` is a template and `g` is its member, satisfying two-phase lookup rules.

---

### Example 8: `typedef` vs `using` for a complex type

<summary>example8.cpp</summary>

```cpp
#include <iostream>
#include <map>
#include <vector>
#include <string>

// Example 8: typedef vs using for a complex type
typedef std::map<std::string, std::vector<int>> MapVec;
using M2 = std::map<std::string, std::vector<int>>;

int main() {
    MapVec mv;
    M2 m2;
    std::cout << "Complex aliases created.\n";
    return 0;
}
```


**Explanation:**
Both `MapVec` and `M2` alias the same complex type. `using` syntax is generally clearer as type complexity grows.

---

### Example 9: `using`-declaration for a type

<summary>example9.cpp</summary>

```cpp
#include <iostream>

namespace C { typedef double real; }
using C::real;

void use_real(real r) { std::cout << "real: " << r << "\n"; }

int main() {
    use_real(3.14);
    return 0;
}
```


**Explanation:**
`using C::real;` imports `C::real` into the global scope as `double`.

---

### Example 10: Implicit conflicts with `using namespace`

<summary>example10.cpp</summary>

```cpp
#include <iostream>

namespace D {
    struct Foo {};
    void Foo() { }
}
using namespace D;

int main() {
    Foo foo_obj; // ambiguous: type vs. function (“most vexing parse”)
    return 0;
}
```


**Explanation:**
Importing both a type and a function named `Foo` causes parsing ambiguity and potential “most vexing parse” issues.

---

## 5. Summary

- **Aliases are synonyms:** A `typedef` or `using` alias is just a new name for an existing type. It does _not_ create a distinct type.
- **Scope matters:** Aliases follow normal C++ lookup rules. Declare aliases in the appropriate namespace or class. To avoid confusion, qualify them when needed (`N::Alias` or `Class::Alias`). In templates, use `typename` when referring to a dependent alias.
- **Prefer `using`:** For modern code, prefer the `using` syntax—especially for templates—since it can do everything `typedef` can and more (alias templates).
- **Avoid name conflicts:** Don’t try to give an alias the same name as an unrelated entity. 
- **Be explicit for ADL:** Remember that ADL ignores alias names. If you rely on associated functions or friends, qualify calls or use `using` declarations accordingly.
- **Clarity:** Use descriptive alias names and keep them in appropriate scopes. This makes code easier to understand. 


---

## 6. Our Experiences

— here —


## 7. Sources

- https://en.cppreference.com/w/cpp/language/type_alias.html
- https://learn.microsoft.com/en-us/cpp/cpp/aliases-and-typedefs-cpp
- https://timsong-cpp.github.io/cppwp/n4140/basic.lookup
