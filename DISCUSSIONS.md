# Discussions

## @kwitaszczyk

Have you considered using libprocstat and libclang (in place of nm) in a separate process created with fork() within the same traced process? This might be a more flexible solution where you don't have to deal with output parsing subject to easy to introduce regressions.

Regarding the unnecessary complications when using libprocstat, would it be possible to filter them out since you have a derivation tree where nodes could be associated with specific libraries?

## @nconnolly1

 I wrote the code to use libprocstat (removed in commit 0ef538eea9540b204a96c6946cd24480587359cc) and considered a number of options including dynamically loading/unloading to get the mapping (left me wondering how to know if it was already there), forking and dynamically loading it in the fork, or just requiring it as part of the linked executable. I finally came to the conclusion that starting an external process wasn't very different to doing a fork or multiple dynamic library loads (libprocstat brings in other libraries including libelf etc) and it's a 'simpler' approach. When I was thinking about this, I didn't have a filter mechanism for the capabilities. In theory, there shouldn't be much leakage into the application, so 'hiding' libprocstat may well be straightforward.

I then wrote the 'load from pipe' functions to load the output from a pipe or path into an in-memory vector which made using an external command trivial. Using 'nm' for the symbols meant that the same code will also run simply on Linux. Using external commands may also provide easier support for other CHERI enabled operating systems. I do understand the point you make about the maintenance liability and I'm very open to revisiting this (keeping the existing mechanism for other platforms as required).

I haven't looked at libclang - I was assuming libelf was the right place to start, but I'll take a look. I also wondered whether the dynamic loader had enough information to be a useful source of external and static symbols without needing to load anything else.

## @kwitaszczyk

Regarding the comment on portability, I agree it would be better not to link against compiler libraries. Especially that procstat cannot be linked as the compiler isn't available in the repository of CheriBSD. I don't know libelf and rtld sufficiently well to advise on that but it sounds like @John Baldwin's question.

---

## @kwitaszczyk

I wonder if it would be more useful to have a standalone program or even better an option to procstat with an API in libprocstat that would provide such functionality. If a user wants to print a derivation tree at a specific point in time at run time, they could attach a debugger to the process, and separately they could use the standalone utility or the library to print the tree.

## @nconnolly1

The intent was that the library could form the basis of a utility accessing either a core dump (if the tag is preserved in them) or a running process - apart from collecting the registers, there is really only one memory access function involved in dereferencing pointers and that could easily be connected through ptrace etc.

---
## @kwitaszczyk

Apart from that, if you'd like people to have an easy way to make use of your utility, we could add it to CheriBSD ports and deliver it to TAP and other CheriBSD users as a package.

## @nconnolly1

I'm very happy for the code to be built as part of CheriBSD ports. I'd need to understand the makefile requirements, but at the moment there's not a lot that will need to be re-done.

---
## @nconnolly1

I'm hoping to gain an understanding of how people would use the utility and the best way to present the information e.g. filtered for a library as you suggest, or 'reverse' questions like 'how can this capability get accessed from the current register set?'

## @kwitaszczyk

When working on bugs in the QEMU user mode I often had to trace back a CPU trace to find an instruction with its operands that created a capability that resulted in a later crash. This was mostly related to heap-allocated memory as statically-allocated memory is supposed to be used close to the place it's allocated. It would be extremely useful if such utility could help to construct not only a derivation tree but also a path from a node to a root of the tree, possibly associating nodes with instructions of a process. I know it sounds like magic but I guess it would be worth to explore.

It would be great to have a way to ask for a subtree of a specific library, or filter out that subtree.
