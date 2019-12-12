# Part 62: Code Cleanup

This version of the compiler is functionally the same as in part 60.
I am using this part to fix up comments, do a bit of code cleanup,
rename some functions and variables etc.

The files here may change over time as I spot more things to fix, but
it will always be functionally equivalent to part 60.

## What's Next

I've been reading up on register allocation, and I think I'll add
a linear scan register allocation mechanism to the compiler. To do
this, though, I need to add an intermediate representation stage.
This will be the goal for the next few stages, but so far I haven't
done anything concrete.
