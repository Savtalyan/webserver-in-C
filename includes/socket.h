#ifndef SOCKET_H
#define SOCKET_H

// returns a listening fd, or -1 on failure
int create_listening_socket(const char *port, const int backlog);

// returns a connected fd, or -1 on failure
int accept_connection(const int listen_fd);

#endif