#include <iostream>
#include <csignal>

#include "common.h"
#include "FTPServer.h"

FTPServer *server;

extern "C" void sighandler(int signal, siginfo_t *info, void *ptr) {
   std::cout << "Received sigaction" << std::endl;
   server->stop();
   exit(-1);
}

void exit_handler() {
   server->stop();
}

void print_usage() {
   std::cout << "Usage: ftp_server [port]" << std::endl;
}

int main(int argc, char **argv) {
   struct sigaction action{};
   action.sa_sigaction = sighandler;
   action.sa_flags = SA_SIGINFO;
   sigaction(SIGINT, &action, nullptr);
   // any port
   if (argc == 1) {
      server = new FTPServer(0);
   } else {
      int port;
      if (sscanf(argv[1], "%i", &port) != 1) {
         print_usage();
         errexit("\tport must be an integer\n");
      }
      server = new FTPServer(port);
   }
   atexit(exit_handler);
   server->run();
}
