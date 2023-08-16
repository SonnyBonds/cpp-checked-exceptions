#include <iostream>
#include "checked_except.h"

// A function declares that it may throw one or more exception types
// by adding an argument of the "Throws" type.
void test_function(Throws<std::logic_error, std::runtime_error> except)
{
    // The Throws object can be used to raise an exception
    // (I'd call it throw if that wasn't a keyword so I can't use it)
    except.raise(std::logic_error("Oh no!"));

    // raise just calls "throw" so it is fully compatible with regular exceptions,
    // and using the "raise" call is just a helper to guarantee you don't 
    // accidentally throw something you haven't declared you're throwing.

    // We can also pass the our own Throws declaration to another function. 
    // This would typically be done when we're just passing the catch 
    // responsibility up the call stack.
    test_function(except);
}

int main()
{
    // try_checked works like regular "try"
    try_checked {
        // ...but inside this block there's a "Throws<...> except" object 
        // available that can passed to functions, to communicate that we 
        // can catch everything it may throw at us. 
        // Not catching all exceptions test_function throws results in a 
        // compile time error. 
        test_function(except);

        // We can also raise/throw exceptions here too if we want to.
        except.raise(std::runtime_error("Hello from runtime_error"));
    }
    // catch_checked works mostly like catch
    catch_checked(const std::runtime_error& e) {
        std::cout << "Caught runtime_error: " << e.what() << std::endl;
    }
    // We can catch a base class and it'll cover all sub classes,
    // just as you'd expect from catch. So, this catch covers the std::logic_error
    // thrown by test_function above.
    catch_checked(const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }; // <-- This semi colon is not there with regular try/catch, but we need it here

    try
    {
        // If we really don't want to pass a Throws object, we can pass BypassExceptionCheck
        // Maybe we know we're handling the exceptions some other ways already.
        test_function(BypassExceptionCheck);
    }
    catch (...)
    { }

    // The whole thing turns into an expression, so we can actually return values as well,
    // which maybe can be nifty idk.
    std::cout << "Printing a return value: " << (try_checked{
        except.raise(std::exception());
        return "Would have been returned from the regular path.\n";
    }
    catch_checked(const std::exception&)
    {
        return "Oh no we hit an exception so this message is returned instead.\n";
    });
}