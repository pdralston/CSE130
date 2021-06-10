Input : Argument count: argc
Input : Array of arguments: argv
Output : returns 0 upon successful completion, non-zero otherwise
if argc == 1
  if (read_file(0) == -1 or close(fd) == -1)
    print error to stderr and exit(errno)
for file in argv[1 -> argc-1]
  if file == '-' and stdin_read == false
    fd = 0
    stdin_read = true
  if file != '-'
    fd = open(file)
  if fd != null
    if (fd == -1 or read_file(fd) == -1 or close(fd) == -1)
      print error to stderr and exit(errno)
exit(0)
