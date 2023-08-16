# Small header for emulating checked exceptions in C++.

In some languages, for example Java, it is possible to declare what exceptions a function may throw, and the compiler can verify that all call sites handle these exceptions, or throws them further up the stack.

In C++ there is no way of doing this; enter [checked_except.h](checked_except.h)

This util header adds a type called Throws<...>, that can be added as an argument to a function as a specification of what exceptions it may throw, e.g.

```c++
void do_something(int a, int b, Throws<std::invalid_argument, std::out_of_range>)
```

To call this function, the caller must provide a corresponding Throws object, communicating that it is catching the exceptions thrown. Creating this manually would already add some exception safety since it give a compile time guarantee that caller/callee is matched, but let's do it automatically.

```c++
// Try a block of code
try_checked {
    // In this block, we have an object named 'except' available that
    // matches exceptions caught by catch_checked statements below,
    // which can be passed to functions
    do_something(1, 2, except);
}
catch_checked(const std::logic_error& e) {
    // Handle exception
}
catch_checked(const std::exception& e) {
    // Handle exception
};
```

...and that's about it, right now. Check out [example.cpp](example.cpp) for a slightly more verbose example.

### Pros
* You are somewhat forced to address exceptions properly.
* Works on top of classic exception handling.

### Cons
* You are somewhat forced to address exceptions properly.
* The extra argument can be a problem, and doesn't work in e.g. operator overloads.
* While most of the code is supposed to be collapsed into nothing at runtime, there _will_ probably be an extra entry in the call stack for each catch, and possibly other runtime effects.
* There are a bit of variadic template shenanigans going on that very well have effects on compile time. 
* try_checked and catch_checked are macros, which may or may not be a problem. It's very possible to do this without the macros, but with a tiny bit manual lambda boilerplate.
* There's an extra ; boi at the end. 

### Future features?
* catch(...) is currently not supported but should be easy to add.
* It is currently not possible to "extend" a Throws object that's passed into a function with more catches inside the function.
* The building blocks that are used for implementing this can be combined in more ways to do some interesting things. The currently shape is designed to look as much as classic try/catch as possible.