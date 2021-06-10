- Perry David Ralston Jr.
- pdralston@ucsc.edu
---

# Assignment 1

rpcserver can be built simply by running make without any arguments.

## Known bugs

write commands complete but they are polluting the buffer for some reason.

read commands have a heisenbug that causes the client to print an extra character when confirming
successful completion of a subsequent task.