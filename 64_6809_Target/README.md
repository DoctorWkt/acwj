# Warren's In-progress Work on a 6809 C Compiler

Basic instructions (for now)!

 - Make sure `as6809` and `ld6809` are on your `PATH`
 - untar `optwcc.tgz` in `/opt`
 - If you want the amd64 backend, you'll need to install QBE:
   https://c9x.me/compile/
 - You will need `/opt/wcc/bin/` on your `PATH`, or at least
   `/opt/wcc/bin/wcc` on your `PATH`. I usually symlink this
   so I can type `wcc` and get it to run.

Now you should be able to `make; make install` which builds
the compiler with the 6809 and QBE backends and installs
them in `/opt/wcc/bin`. The header files are copied into
`/opt/wcc/include` and the 6809 `crt0.o` into `/opt/wcc/lib/6809`.

Now you should be able to:

 - `make test` to run the amd64 tests (assuming you have QBE)
 - `make 6test` to run the 6809 tests
 - `make triple` to compile the amd64 compiler with itself
    and check that the binary checksums are the same.

`make clean` should clean up everything.
