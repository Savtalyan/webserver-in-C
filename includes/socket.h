#ifndef SOCKET_H
#define SOCKET_H 

// TODO: add defenitions

int create_listening_socket(const char *port, const int backlog);

int accept_connection(const int listen_fd);

#endif