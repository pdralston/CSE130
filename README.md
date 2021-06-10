# Programming assignments for CSE 130, Fall 2020 (Miller)

This repository must be used for all of the programming assignments in CSE 130, Fall 2020 (Miller).
We will only grade code and documentation (design document, writeup) that has been committed
to this repository.  Code and documentation must be committed in the appropriate directory
(*e.g.*, `asgn0` for Assignment 0). Please read the
[assignment acceptance criteria](https://canvas.ucsc.edu/courses/36179/pages/assignment-acceptance-criteria)
for details on to know if your assignment is ready to be submitted.

## Root directory files

The root directory contains two files, `.gitignore` and `.gitlab-ci.yml`. The first file,
`.gitignore` contains patterns for files that shouldn't be committed, and will be ignored
by `git`. While we tried to make it complete, we probably missed some files; that doesn't
mean that they're allowed.

The second file, `.gitlab-ci.yml`, tells the server how to run automated tests on your repository.
***DO NOT MODIFY THIS FILE***. Committing a modified version of this file will, at a minimum,
cause issues with automated processing, and (depending on what you do) could be considered
academic misconduct. If you accidentally modify or delete the file, ***DON'T PANIC***.
If you haven't committed the change using `git commit`, you can always recover it by running
`git checkout HEAD .gitlab-ci.yml`. And even if you *have* committed the changes,
[you can recover it](https://www.git-tower.com/learn/git/faq/restoring-deleted-files):
that's the nice thing about `git`.
