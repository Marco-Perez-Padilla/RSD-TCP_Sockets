//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                        Main class of the FTP server
// 
//                        PE104-11:
//                          Juan Xavier León Pérez
//                          Marco Pérez Padilla
//****************************************************************************

#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <list>
#include <iostream>
#include "common.h"
#include "FTPServer.h"
#include "ClientConnection.h"


/**
 * @brief Function that defines a TCP socket, managing possible errors.
 *        If port is 0, a random available port is assigned.
 * @param port Number of the port to be used in the connection (0 = random).
 * @return Defined socket, or -1 on error.
 */
int define_socket_TCP(int port = 0) {
  // Create the socket, associated to TCP
  int s = socket(AF_INET, SOCK_STREAM, 0); 

  // If it couldn't be created, return -1 code error
  if (s < 0) {
    errexit("No se pudo crear el socket: %s<\r\n", strerror(errno));
  }

  // Declare the struct to define the address and port of the socket
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  
  // Fill fields
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(port); 

  // Bind the socket
  if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    errexit("No se pudo hacer bind: %s\r\n", strerror(errno));
  }

  // If port was 0, retrieve the assigned port
  if (port == 0) {
    socklen_t len = sizeof(sin);
    if (getsockname(s, (sockaddr*)&sin, &len) >= 0) {
      port = ntohs(sin.sin_port);
    } else {
      errexit("getsockname: %s\r\n", strerror(errno));
    }
  } 
    
  // Listen on the socket
  if (listen(s, 5) < 0) {
    errexit("No se pudo hacer listen: %s\r\n", strerror(errno));
  }

  std::cout << "Listening on port: " << port << std::endl;

  return s;
}


// This function is executed when the thread is executed.
void *run_client_connection(void *c) {
  ClientConnection *connection = (ClientConnection *) c;
  connection->WaitForRequests();
  return nullptr;
}

FTPServer::FTPServer(int port) {
  this->port = port;
}

// Stops the server
void FTPServer::stop() {
  close(msock);
  shutdown(msock, SHUT_RDWR);
}

// Starts of the server
void FTPServer::run() {
  struct sockaddr_in fsin;
  int ssock;
  socklen_t alen = sizeof(fsin);
  msock = define_socket_TCP(port); // This function must be implemented by you
  while (true) {
    pthread_t thread;
    ssock = accept(msock, (struct sockaddr *) &fsin, &alen);
    if (ssock < 0) {
      errexit("Fallo en el accept: %s\n", strerror(errno));
    }  
    auto *connection = new ClientConnection(ssock);
    // Here a thread is created in order to process multiple requests simultaneously
    pthread_create(&thread, nullptr, run_client_connection, (void *) connection);
  }
}