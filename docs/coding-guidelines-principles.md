# General Coding Guidelines Principles

## Motivation
The main purpose of adopting a set of coding guidelines is to facilitate the understanding of the software we write.
This enables an engineer to understand faster the code written by another engineer, which enables them to figure out faster how to debug or modify that code.

There are two fundamental things that an engineer needs to understand about each piece of code:
* Purpose - what it is meant to accomplish.
* Method - how it attempts to accomplish its purpose.

But, besides these two aspects, an engineer may often need to understand other aspects:
* Customization - how can the code be configured to run in different modes.
* Supportability - how can the code be debugged.
* Extensibility - how was the code meant to be changed to address new requirements.

Hence, our coding guidelines will be meant to facilitate the understanding of one of more of
these aspects.

## Guiding Principles
These are the principles that guide our guidelines.

1. Search to achieve clarity and make your intent evident to the reader.
    1. If you can make your intentions clearer, do it!
    2. Think of it as not wasting the brain-cycles of the reader.
2. Keep it simple (KISS - keep it simple, stupid).
    1. Everything else being the same, prefer simpler solutions.
        1. Why? Because simpler approaches are easier to understand than more complicated ones.
    2. Applied to the guidelines themselves, this principle also tells us to avoid rules that are complex to implement. For example, brittle formatting rules that require the coder to carefully indent lines should be discarded in favor of indentation rules that are easily enforced by editors like VSCode or emacs.
    3. This should not be confused with writing compact code.
        1. In particular, avoid writing statements that do multiple different operations or chain many different operations. Such statements are difficult to debug.
    4. Do not optimize unless there is a real need to.
        1. Optimizations often make the code more complicated. Do them only when the gains are worth it.
3. Be declarative.
    1. This is about picking names that clearly state the purpose of the classes/methods/variables.
        1. Do not indicate the method, unless relevant. This will avoid the need to change the name if the method changes.
4. Keep related stuff together and separate less-related stuff.
    1. This applies both at code level as an object-oriented principle, as well as at code formatting level.
5. Emphasize the more important aspects over the less important ones.
    1. This applies to naming or formatting. For example, the operators of an expression are more important than the operands, so if an expression is broken over more than one line, the continuing lines should start with an operator rather than an operand.
6. Be consistent.
    1. This applies at many levels. With consistency we gain several benefits:
        1. Easier recognition of repeated patterns.
    2. More reliable results when searching for keywords.
        1. This falls apart if we use inconsistent terminology for the same concept.
7. Do not explain with comments what you can explain in code.
    1. Basically, if you can write the code in such a way as to make it tell what it does, then you can eliminate the need to comment it.
    2. It can help to provide redundant comments as a way of providing a higher level overview of what is done in code.
8. Plan your code to be resilient to different usage. Do not rely on aspects that you cannot control.
    1. Code and calling patterns change. A method that is called in a single place today may get called in ten places tomorrow - do not rely on assumptions that can change easily.

**If it looks like following these principles makes your code look strange, then you’re probably missing a way of refactoring your code.**
