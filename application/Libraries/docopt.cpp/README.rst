``docopt.cpp``: A C++11 Port
============================

Contents
--------

.. contents::
  :local:
  :depth: 3

docopt creates *beautiful* command-line interfaces
--------------------------------------------------

Isn't it awesome how ``getopt`` (and ``boost::program_options`` for you fancy
folk!) generate help messages based on your code?! These timeless functions
have been around for decades and have proven we don't need anything better, right?

*Hell no!*  You know what's awesome?  It's when the option parser *is*
generated based on the beautiful help message that you write yourself!
This way you don't need to write this stupid repeatable parser-code,
and instead can write only the help message--*the way you want it*.

**docopt** helps you create most beautiful command-line interfaces
*easily*:

.. code:: c++

    #include "docopt.h"

    #include <iostream>

    static const char USAGE[] =
    R"(Naval Fate.

        Usage:
          naval_fate ship new <name>...
          naval_fate ship <name> move <x> <y> [--speed=<kn>]
          naval_fate ship shoot <x> <y>
          naval_fate mine (set|remove) <x> <y> [--moored | --drifting]
          naval_fate (-h | --help)
          naval_fate --version

        Options:
          -h --help     Show this screen.
          --version     Show version.
          --speed=<kn>  Speed in knots [default: 10].
          --moored      Moored (anchored) mine.
          --drifting    Drifting mine.
    )";

    int main(int argc, const char** argv)
    {
        std::map<std::string, docopt::value> args
            = docopt::docopt(USAGE,
                             { argv + 1, argv + argc },
                             true,               // show help if requested
                             "Naval Fate 2.0");  // version string

        for(auto const& arg : args) {
            std::cout << arg.first <<  arg.second << std::endl;
        }

        return 0;
    }

Beat that! The option parser is generated based on the docstring above
that is passed to ``docopt::docopt`` function.  ``docopt`` parses the usage
pattern (``"Usage: ..."``) and option descriptions (lines starting
with a dash "``-``") and ensures that the program invocation matches the
usage pattern; it parses options, arguments and commands based on
that. The basic idea is that *a good help message has all necessary
information in it to make a parser*.

Getting and using
-----------------

To get *docopt.cpp*, the simplest is to use `Conda <https://github.com/conda-forge/docopt.cpp-feedstock>`_::

    conda install -c conda-forge docopt.cpp

Alternatively manual installation is done using (unix)::

    git clone
    cmake .
    make install

To link *docopt.cpp*, the simplest is to use CMake. The general structure of your
``CMakeLists.txt`` would be as follows::

    cmake_minimum_required(VERSION 3.1)

    project(example)

    find_package(docopt COMPONENTS CXX REQUIRED)
    include_directories(${DOCOPT_INCLUDE_DIRS})

    add_executable(${PROJECT_NAME} ...)

    target_link_libraries(${PROJECT_NAME} docopt)

C++11 port details
------------------

This is a port of the ``docopt.py`` module (https://github.com/docopt/docopt),
and we have tried to maintain full feature parity (and code structure) as the
original.

This port is written in C++11 and also requires a good C++11 standard library
(in particular, one with ``regex`` support). The following compilers are known
to work with docopt:

- Clang 3.3 and later
- GCC 4.9
- Visual C++ 2015 RC

GCC-4.8 can work, but the std::regex module needs to be replaced with ``Boost.Regex``.
In that case, you will need to define ``DOCTOPT_USE_BOOST_REGEX`` when compiling
docopt, and link your code with the appropriated Boost libraries. A relatively
recent version of Boost is needed: 1.55 works, but 1.46 does not for example.

This port is licensed under the MIT license, just like the original module.
However, we are also dual-licensing this code under the Boost License, version 1.0,
as this is a popular C++ license. The licenses are similar and you are free to
use this code under the terms of either license.

The differences from the Python port are:

* the addition of a ``docopt_parse`` function, which does not terminate
  the program on error
* a ``docopt::value`` type to hold the various value types that can be parsed.
  We considered using boost::variant, but it seems better to have no external
  dependencies (beyond a good STL).
* because C++ is statically-typed and Python is not, we had to make some
  changes to the interfaces of the internal parse tree types.
* because ``std::regex`` does not have an equivalent to Python's regex.split,
  some of the regex's had to be restructured and additional loops used.

API
---

.. code:: c++

    docopt::docopt(doc, argv, help /* =true */, version /* ="" */, options_first /* =false */)

``docopt`` takes 2 required and 3 optional arguments:

- ``doc`` is a string that contains a **help message** that will be parsed to
  create the option parser.  The simple rules of how to write such a
  help message are given in next sections.  Here is a quick example of
  such a string (note that this example uses the "raw string literal" feature
  that was added to C++11):

.. code:: c++

    R"(Usage: my_program [-hso FILE] [--quiet | --verbose] [INPUT ...]

    -h --help    show this
    -s --sorted  sorted output
    -o FILE      specify output file [default: ./test.txt]
    --quiet      print less text
    --verbose    print more text
    )"

- ``argv`` is a vector of strings representing the args passed. Although
  main usually takes a ``(int argc, const char** argv)`` pair, you can
  pass the value ``{argv+1, argv+argc}`` to generate the vector automatically.
  (Note we skip the argv[0] argument!) Alternatively you can supply a list of
  strings like ``{ "--verbose", "-o", "hai.txt" }``.

- ``help``, by default ``true``, specifies whether the parser should
  automatically print the help message (supplied as ``doc``) and
  terminate, in case ``-h`` or ``--help`` option is encountered
  (options should exist in usage pattern, more on that below). If you
  want to handle ``-h`` or ``--help`` options manually (as other
  options), set ``help=false``.

- ``version``, by default empty, is an optional argument that
  specifies the version of your program. If supplied, then, (assuming
  ``--version`` option is mentioned in usage pattern) when parser
  encounters the ``--version`` option, it will print the supplied
  version and terminate.  ``version`` could be any printable object,
  but most likely a string, e.g. ``"2.1.0rc1"``.

    Note, when ``docopt`` is set to automatically handle ``-h``,
    ``--help`` and ``--version`` options, you still need to mention
    them in usage pattern for this to work (also so your users to
    know about them!)

- ``options_first``, by default ``false``.  If set to ``true`` will
  disallow mixing options and positional argument.  I.e. after first
  positional argument, all arguments will be interpreted as positional
  even if the look like options.  This can be used for strict
  compatibility with POSIX, or if you want to dispatch your arguments
  to other programs.

The **return** value is a ``map<string, docopt::value>`` with options,
arguments and commands as keys, spelled exactly like in your help message.
Long versions of options are given priority. For example, if you invoke the
top example as::

    naval_fate ship Guardian move 100 150 --speed=15

the return dictionary will be:

.. code:: python

    {"--drifting": false,    "mine": false,
     "--help": false,        "move": true,
     "--moored": false,      "new": false,
     "--speed": "15",        "remove": false,
     "--version": false,     "set": false,
     "<name>": ["Guardian"], "ship": true,
     "<x>": "100",           "shoot": false,
     "<y>": "150"}

If any parsing error (in either the usage, or due to incorrect user inputs) is
encountered, the program will exit with exit code -1.

Note that there is another function that does not exit on error, and instead will
propagate an exception that you can catch and process as you like. See the docopt.h file
for information on the exceptions and usage:

.. code:: c++

    docopt::docopt_parse(doc, argv, help /* =true */, version /* =true */, options_first /* =false)

Help message format
-------------------

Help message consists of 2 parts:

- Usage pattern, e.g.::

    Usage: my_program [-hso FILE] [--quiet | --verbose] [INPUT ...]

- Option descriptions, e.g.::

    -h --help    show this
    -s --sorted  sorted output
    -o FILE      specify output file [default: ./test.txt]
    --quiet      print less text
    --verbose    print more text

Their format is described below; other text is ignored.

Usage pattern format
--------------------

**Usage pattern** is a substring of ``doc`` that starts with
``usage:`` (case *insensitive*) and ends with a *visibly* empty line.
Minimum example:

.. code:: python

    """Usage: my_program

    """

The first word after ``usage:`` is interpreted as your program's name.
You can specify your program's name several times to signify several
exclusive patterns:

.. code:: python

    """Usage: my_program FILE
              my_program COUNT FILE

    """

Each pattern can consist of the following elements:

- **<arguments>**, **ARGUMENTS**. Arguments are specified as either
  upper-case words, e.g. ``my_program CONTENT-PATH`` or words
  surrounded by angular brackets: ``my_program <content-path>``.
- **--options**.  Options are words started with dash (``-``), e.g.
  ``--output``, ``-o``.  You can "stack" several of one-letter
  options, e.g. ``-oiv`` which will be the same as ``-o -i -v``. The
  options can have arguments, e.g.  ``--input=FILE`` or ``-i FILE`` or
  even ``-iFILE``. However it is important that you specify option
  descriptions if you want your option to have an argument, a default
  value, or specify synonymous short/long versions of the option (see
  next section on option descriptions).
- **commands** are words that do *not* follow the described above
  conventions of ``--options`` or ``<arguments>`` or ``ARGUMENTS``,
  plus two special commands: dash "``-``" and double dash "``--``"
  (see below).

Use the following constructs to specify patterns:

- **[ ]** (brackets) **optional** elements.  e.g.: ``my_program
  [-hvqo FILE]``
- **( )** (parens) **required** elements.  All elements that are *not*
  put in **[ ]** are also required, e.g.: ``my_program
  --path=<path> <file>...`` is the same as ``my_program
  (--path=<path> <file>...)``.  (Note, "required options" might be not
  a good idea for your users).
- **|** (pipe) **mutually exclusive** elements. Group them using **(
  )** if one of the mutually exclusive elements is required:
  ``my_program (--clockwise | --counter-clockwise) TIME``. Group
  them using **[ ]** if none of the mutually exclusive elements are
  required: ``my_program [--left | --right]``.
- **...** (ellipsis) **one or more** elements. To specify that
  arbitrary number of repeating elements could be accepted, use
  ellipsis (``...``), e.g.  ``my_program FILE ...`` means one or
  more ``FILE``-s are accepted.  If you want to accept zero or more
  elements, use brackets, e.g.: ``my_program [FILE ...]``. Ellipsis
  works as a unary operator on the expression to the left.
- **[options]** (case sensitive) shortcut for any options.  You can
  use it if you want to specify that the usage pattern could be
  provided with any options defined below in the option-descriptions
  and do not want to enumerate them all in usage-pattern.
- "``[--]``". Double dash "``--``" is used by convention to separate
  positional arguments that can be mistaken for options. In order to
  support this convention add "``[--]``" to your usage patterns.
- "``[-]``". Single dash "``-``" is used by convention to signify that
  ``stdin`` is used instead of a file. To support this add "``[-]``"
  to your usage patterns. "``-``" acts as a normal command.

If your pattern allows to match argument-less option (a flag) several
times::

    Usage: my_program [-v | -vv | -vvv]

then number of occurrences of the option will be counted. I.e.
``args['-v']`` will be ``2`` if program was invoked as ``my_program
-vv``. Same works for commands.

If your usage pattern allows to match same-named option with argument
or positional argument several times, the matched arguments will be
collected into a list::

    Usage: my_program <file> <file> --path=<path>...

I.e. invoked with ``my_program file1 file2 --path=./here
--path=./there`` the returned dict will contain ``args['<file>'] ==
['file1', 'file2']`` and ``args['--path'] == ['./here', './there']``.

Option descriptions format
--------------------------

**Option descriptions** consist of a list of options that you put
below your usage patterns.

It is necessary to list option descriptions in order to specify:

- synonymous short and long options,
- if an option has an argument,
- if option's argument has a default value.

The rules are as follows:

- Every line in ``doc`` that starts with ``-`` or ``--`` (not counting
  spaces) is treated as an option description, e.g.::

    Options:
      --verbose   # GOOD
      -o FILE     # GOOD
    Other: --bad  # BAD, line does not start with dash "-"

- To specify that option has an argument, put a word describing that
  argument after space (or equals "``=``" sign) as shown below. Follow
  either <angular-brackets> or UPPER-CASE convention for options'
  arguments.  You can use comma if you want to separate options. In
  the example below, both lines are valid. However you are recommended
  to stick to a single style.::

    -o FILE --output=FILE       # without comma, with "=" sign
    -i <file>, --input <file>   # with comma, without "=" sing

- Use two spaces to separate options with their informal description::

    --verbose More text.   # BAD, will be treated as if verbose option had
                           # an argument "More", so use 2 spaces instead
    -q        Quit.        # GOOD
    -o FILE   Output file. # GOOD
    --stdout  Use stdout.  # GOOD, 2 spaces

- If you want to set a default value for an option with an argument,
  put it into the option-description, in form ``[default:
  <my-default-value>]``::

    --coefficient=K  The K coefficient [default: 2.95]
    --output=FILE    Output file [default: test.txt]
    --directory=DIR  Some directory [default: ./]

- If the option is not repeatable, the value inside ``[default: ...]``
  will be interpreted as string.  If it *is* repeatable, it will be
  split into a list on whitespace::

    Usage: my_program [--repeatable=<arg> --repeatable=<arg>]
                         [--another-repeatable=<arg>]...
                         [--not-repeatable=<arg>]

    # will be ['./here', './there']
    --repeatable=<arg>          [default: ./here ./there]

    # will be ['./here']
    --another-repeatable=<arg>  [default: ./here]

    # will be './here ./there', because it is not repeatable
    --not-repeatable=<arg>      [default: ./here ./there]

Examples
--------

We have an extensive list of `examples
<https://github.com/docopt/docopt/tree/master/examples>`_ which cover
every aspect of functionality of **docopt**.  Try them out, read the
source if in doubt.

There are also very interesting applications and ideas at that page.
Check out the sister project for more information!

Subparsers, multi-level help and *huge* applications (like git)
---------------------------------------------------------------

If you want to split your usage-pattern into several, implement
multi-level help (with separate help-screen for each subcommand),
want to interface with existing scripts that don't use **docopt**, or
you're building the next "git", you will need the new ``options_first``
parameter (described in API section above). To get you started quickly
we implemented a subset of git command-line interface as an example:
`examples/git
<https://github.com/docopt/docopt/tree/master/examples/git>`_

Compiling the example / Running the tests
-----------------------------------------

The original Python module includes some language-agnostic unit tests,
and these can be run with this port as well.

The tests are a Python driver that uses the testcases.docopt file to then invoke
a C++ test case runner (run_testcase.cpp)::

  $ clang++ --std=c++11 --stdlib=libc++ docopt.cpp run_testcase.cpp -o run_testcase
  $ python run_tests.py
  PASS (175)

You can also compile the example shown at the start (included as example.cpp)::

  $ clang++ --std=c++11 --stdlib=libc++ -I . docopt.cpp examples/naval_fate.cpp -o naval_fate
  $ ./naval_fate --help
   [ ... ]
  $ ./naval_fate ship Guardian move 100 150 --speed=15
  --drifting: false
  --help: false
  --moored: false
  --speed: "15"
  --version: false
  <name>: ["Guardian"]
  <x>: "100"
  <y>: "150"
  mine: false
  move: true
  new: false
  remove: false
  set: false
  ship: true
  shoot: false

Development
-----------

Comments and suggestions are *very* welcome! If you find issues, please
file them and help improve our code!

Please note, however, that we have tried to stay true to the original
Python code. If you have any major patches, structural changes, or new features,
we might want to first negotiate these changes into the Python code first.
However, bring it up! Let's hear it!

Changelog
---------

**docopt** follows `semantic versioning <http://semver.org>`_.  The
first release with stable API will be 1.0.0 (soon).

- 0.6.2 Bugfix release (still based on docopt 0.6.1)
- 0.6.1 The initial C++ port of docopt.py (based on docopt 0.6.1)
