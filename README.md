# CheriTree

CheriTree provides a library to help understand the impact of library compartmentalisation on an application. It prints a capability derivation tree for the running application, by recursively searching through the registers and stack, looking for all valid capabilities.

[rtegrity](https://rtegrity.com/) has developed the project as part of the [Digital Security by Design](https://www.dsbd.tech/) Technology Access Program.

# In this ReadMe

* [Source Code](#source)
* [Design Notes](#design)
* [Getting Started](#start)
* [Runtime Prerequisites](#prereq)
* [Current Status](#status)
* [Limitations](#limitations)
* [Known Issues](#issues)
* [Unit Tests](#unit)
* [Example Code](#examples)
* [Contributing](#contrib)
* [Acknowledgements](#acknowledge)
* [Core Maintainers](#core)

<a id="source"></a>
## Source Code

The source code can be obtained using:
~~~{.sh}
git clone https://github.com/rtegrity/cheritree
~~~

<a id="design"></a>
## Design Notes

CheriTree is implemented as a shared library and a stub that is linked with the application. Within the shared library, offsets are used rather than pointers, to minimise the number of capabilities introduced into the application.

The library builds an in memory list of the mapped segments and loads the associated symbol tables. On FreeBSD this is done using the output from the ___procstat___ and ___nm___ commands. An earier version used ___libprocstat___, but the required libraries significantly complicated the address space, so the simpler design of an external command was used instead. On Linux, the /proc filesystem is used to obtain the mapped segments, but this is not enabled by default on CheriBSD and doesn't appear to have capability information added yet.

During execution, segments (especially stacks) can grow and new libraries or mappings can be added. CheriTree attempts to handle this by reloading the mapping list if necessary.

When ___cheritree_print_capabilities()___ is called, the stack is adjusted by 1MB to preserve any residual stack capabilities and then all of the registers are saved. On return, the registers are restored, making the call suitable for use at arbitrary points in assember code.

The portion of the stack associated with running ___cheritree_print_capabilities()___ is deliberately omitted from the output to aid clarity.

The output currently goes to _stdout_, but the design will support a programmatic interface. In particular, the intent is to have a call that identifies capabilities that are accessible from the current compartment, but don't belong to it.

<a id="start"></a>
## Getting Started

The project can be built by running ___make___ at the top level. Only native CheriBSD builds on Morello hardware have been tested.

To include the library with an existing application, link with both ___cheritreestub.a___ (contains assembler wrappers to preserve the state) and ___cheritree.so___. The capability tree can be seen by calling ___cheritree_print_capabilities()___, which is defined in ___cheritree.h___.

Optionally, a call to ___cheritree_init()___ can be added before use. If there are multiple shared libraries, calling this from each one will enable CheriTree to identify the associated stack.

<a id="prereq"></a>
## Runtime Prerequisites

CheriBSD 22.12 or later is required for library compartmentalisation support.

LD_LIBRARY_PATH must be set to include the compiled binaries. For example:

~~~{.sh}
export LD_LIBRARY_PATH=.
export LD_64_LIBRARY_PATH=.
export LD_C18N_LIBRARY_PATH=.
~~~

<a id="status"></a>
## Current Status

The project is at an alpha stage, but is being made available for wider experimentation.
In testing, it reliably produces a list of capabilities and the associated symbol (where known).
It is still work in progress and is subject to change.

<a id="limitations"></a>
## Limitations

* The make system is rudimentary and defaults to a full rebuild, but it only takes a few seconds.
* There is no install script or packaging.
* Only _stdout_ is currently supported, but the code builds an in-memory list that could be used in future.
* There are currently no unit tests associated with the project.
* Portions of the code have been run on Linux, but not on a CHERI enabled build.

<a id="issues"></a>
## Known Issues

* None.

<a id="unit"></a>
## Unit Tests

Not currently implemented.

<a id="examples"></a>
## Example Code

Running ___make___ will build the program in ./example which has two shared libraries.

For a purecap build without library compartmentalisation, run:

~~~{.sh}
./shared-example
~~~

For a purecap build with library compartmentalisation, run:
~~~{.sh}
./c18n-example
~~~

<a id="contrib"></a>
## Contributing

Contributions are welcome! In the initial stages of the project, please contact the [maintainers](https://github.com/rtegrity/cheritree/blob/main/MAINTAINERS.md) directly by email or via CHERI Slack.

<a id="acknowledge"></a>
## Acknowledgements
[rtegrity](https://rtegrity.com/) would like to acknowledge the support of the [Digital Security by Design](https://www.dsbd.tech/) Technology Access Program.

<a id="core"></a>
## Core Maintainers

The [core maintainers](https://github.com/rtegrity/cheritree/blob/main/MAINTAINERS.md) primary responsibility is to provide technical oversight for the project. The current list includes:
* [Nick Connolly](https://github.com/nconnolly1), [rtegrity](https://rtegrity.com/)
