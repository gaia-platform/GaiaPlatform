This page collects common guidelines for error messages. These guidelines also apply to assert or log messages.

* Follow proper capitalization and punctuation.
  * Good:
    ```
    Process failed.

    DDL file not found!
    ```
  * Bad:
    ```
    process failed

    ddl file not found.
    ```
* Use single-quotes (not double-quotes) when referencing names or values that serve as identifiers. Do not quote values that have intrinsic meaning.

  * Good:

    Cannot find table 'student'!

    Type '32' does not exist!

    Payload size 4068 exceeds maximum size 2048!

  * Bad:

    Cannot find table "student"!

    Type 32 does not exist!

* When referencing functions, append parentheses after their name, to make it clear that they represent function calls.
  * Good:

    listen() failed!

  * Bad:

    listen failed!

* Use constants for messages that are used in many places, so that if they need to be updated that can be done in a single place.
