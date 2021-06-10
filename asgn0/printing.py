Input : File descriptor: fd
Output : returns 0 upon successful completion, -1 otherwise

bytes_read = read(fd, buffer, BUFFER_SIZE)
while bytes_read > 0
  if write(STDOUT, buffer, BUFFER_SIZE) == -1
    return(-1)
if bytes_read = -1 or write(STDOUT, buffer, BUFF_SIZE) == -1
  return(-1)
return(0)
