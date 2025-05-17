//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                    This class processes an FTP transaction.
// 
//                        PE104-11:
//                          Juan Xavier León Pérez
//                          Marco Pérez Padilla
//****************************************************************************

#include <cstring>
#include <cstdio>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <iostream>
#include "ClientConnection.h"
#include "FTPServer.h"

ClientConnection::ClientConnection(int s) {
   control_socket = s;  // Store the client socket
   control_fd = fdopen(s, "a+"); // Convert the socket to a FILE* for easier input/output with fscanf/fprintf
   if (control_fd == nullptr) {
      std::cout << "Connection closed" << std::endl; 
      close(control_socket);  // Close the socket if fdopen fails
      ok = false;
      return;
   }
   ok = true;  // Mark the connection as valid
   data_socket = -1; // No data connection yet
   stop_server = false; // Server should keep running
   };

ClientConnection::~ClientConnection() {
   fclose(control_fd);
   close(control_socket);
}

int connect_TCP(uint32_t address, uint16_t port) {
   // TODO: Implement your code to define a socket here
   // Create a socket address structure and set address family and port
   struct sockaddr_in sin;
   memset(&sin, 0, sizeof(sin));
   sin.sin_family = AF_INET;
   sin.sin_port = htons(port);
   sin.sin_addr.s_addr = address; 

   // Create a TCP socket
   int s = socket(AF_INET, SOCK_STREAM, 0);
   if (s < 0) {
      perror("Error al crear el socket");
      return -1;
   }

   // Attempt to connect to the server
   if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
      perror("Error al conectar con el servidor");
      close(s);
      return -1;
   }

   return s;   // Return the connected socket descriptor
}

void ClientConnection::stop() {
   close(data_socket);
   close(control_socket);
   stop_server = true;
}

#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.
void ClientConnection::WaitForRequests() {
   bool logged_in = false;
   uint32_t ip = -1;
   uint16_t port = -1;
   if (!ok) {
      return;
   }
   fprintf(control_fd, "220 Service ready\n");
   fflush(control_fd);
   while (!stop_server) {
      fscanf(control_fd, "%s", command);
      if (COMMAND("USER")) {
         fscanf(control_fd, "%s", arg);
         fprintf(control_fd, "331 User name ok, need password\n");
         fflush(control_fd);
      } else if (COMMAND("PWD")) {
         fprintf(control_fd, "257 \"%s\" is current directory.\r\n","/");  
         fflush(control_fd);
      } else if (COMMAND("PASS")) {
         fscanf(control_fd, "%s", arg);
         if (strcmp(arg, "1234") == 0) {
            fprintf(control_fd, "230 User logged in\n");
            fflush(control_fd);
            logged_in = true;
         } else {
            fprintf(control_fd, "530 Not logged in.\n");
            fflush(control_fd);
            stop_server = true;
            break;
         }
      } else if (COMMAND("PORT")) {
         int h0, h1, h2, h3;
         int p0, p1;
         fscanf(control_fd, "%d,%d,%d,%d,%d,%d", &h0, &h1, &h2, &h3, &p0, &p1);  // Read client's IP and port in PORT command format: h0,h1,h2,h3,p0,p1
         uint32_t address = h3 << 24 | h2 << 16 | h1 << 8 | h0;   // Pack IP into 32-bit value 
         uint32_t port = p0 << 8 | p1; // Pack port from two 8-bit values
         data_socket = connect_TCP(address, port); // Try to connect to client's data socket
         fprintf(control_fd, "200 OK,\r\n");
         fflush(control_fd);         
      } else if (COMMAND("PASV")) {
         int socket, p0, p1;
         uint16_t port; 

         struct sockaddr_in sin;
         socklen_t slen;
         slen = sizeof(sin);

         socket = define_socket_TCP(0); // Create socket on a random port for passive mode
         getsockname(socket,(struct sockaddr *)&sin, &slen); // Get the auto-assigned port number
            
         // Split port into two bytes to send to the client
         port = sin.sin_port;
         p0 = (port & 0xff);
         p1 = (port >> 8);

         // Send IP and port to client in PASV format
         fprintf(control_fd, "227 Entering Passive Mode (127,0,0,1,%d,%d).\r\n", p0, p1);
         fflush(control_fd);

         data_socket = accept(socket,(struct sockaddr *)&sin, &slen);   // Wait for client to connect to the data socket

      } else if (COMMAND("STOR")) {
         // TODO: To be implemented by students
         fscanf(control_fd, "%s", arg);   // Retrieve the file name the client wants to upload
         // If the IP is valid, attempt to connect using the specified IP and port
         if (ip != (uint32_t)-1) { 
            data_socket = connect_TCP(ip,port); 
            ip = -1; // Reset IP after connection
         }

         // If the data connection cannot be opened, send an error message
         if (data_socket < 0) { 
            fprintf(control_fd,"425 Can't open data connection.\r\n"); 
            fflush(control_fd);  // Ensure the message is sent to the client
            continue;   // Continue to the next command
         }

         // Attempt to open the file for writing
         FILE *file = fopen(arg,"wb");
         if (!file) { 
            fprintf(control_fd,"550 Cannot open file.\r\n");   // If the file cannot be opened
            fflush(control_fd);
            close(data_socket);  // Close the data socket
            data_socket=-1;   // Reset the data socket
            continue; 
         }
         
         // Confirm to the client that the transfer is starting
         fprintf(control_fd,"150 Opening data connection for STOR\r\n");
         fflush(control_fd);

         char buf[4096]; 
         int r;

         // Read data from the data socket and write to the file
         while ((r = read(data_socket,buf,sizeof(buf)))>0) {
            fwrite(buf,1,r,file);
         }

         // Close the file and data socket after the transfer
         fclose(file); 
         close(data_socket); 
         data_socket=-1;

         // Confirm that the transfer has completed successfully
         fprintf(control_fd,"226 Transfer complete\r\n");
         fflush(control_fd);
         
      } else if (COMMAND("RETR")) {
         // TODO: To be implemented by students
         fscanf(control_fd, "%s", arg);   // Read the filename to be retrieved

         // If a valid IP was stored, try to connect using it
         if (ip != (uint32_t)-1) { 
            data_socket = connect_TCP(ip,port); 
            ip=-1; 
         }

         // Check if data connection succeeded
         if (data_socket < 0) { 
            fprintf(control_fd,"425 Can't open data connection.\r\n"); 
            fflush(control_fd);
            continue; 
         }

         // Try opening the requested file for reading
         FILE *file = fopen(arg,"rb");
         if (!file) { 
            fprintf(control_fd,"550 Cannot open file.\r\n"); 
            fflush(control_fd);
            close(data_socket); 
            data_socket=-1; 
            continue; 
         }

         // Notify the client that data transfer is starting
         fprintf(control_fd,"150 Opening data connection for RETR\r\n");
         fflush(control_fd);

         // Read from file and send to client over data socket
         char buf[4096]; 
         int r;
         while ((r=fread(buf,1,sizeof(buf),file))>0) {
            write(data_socket,buf,r);
         }

         // Close file and data socket after transfer
         fclose(file);
         close(data_socket); 
         data_socket=-1;

         fprintf(control_fd,"226 Transfer complete\r\n");
         fflush(control_fd);
      } else if (COMMAND("LIST")) {
         // TODO: To be implemented by students
         if (logged_in == true) {
            // Notify client that data connection is ready
            fprintf(control_fd, "125 Data connection already open; transfer starting.\r\n");
            fflush(control_fd);

            // Open current directory
            DIR *d;
            struct dirent* arc_dir;
            d = opendir(".");

            // If directory can't be opened, report an error
            if (d == nullptr) {
               fprintf(control_fd, "425 Can't open data connection.\r\n");
               fflush(control_fd);
            } else {
               while ((arc_dir = readdir(d)) != nullptr) {  // Send each file/directory name over data connection
                  send(data_socket, arc_dir->d_name, strlen(arc_dir->d_name), 0);
                  send(data_socket, "\n", 1, 0);
               }
               // Confirm success
               fprintf(control_fd, "250 Requested file action okay, completed.\r\n");
               fflush(control_fd);
            }
            // Close directory and data connection
            closedir(d);
            close(data_socket);
         } else if (logged_in == false) {
            // User is not authenticated
            fprintf(control_fd, "530 Not logged in.\r\n");
            fflush(control_fd);
         } else {
            // Unexpected state
            fprintf(control_fd, "450 Error, system busy.\r\n");
            fflush(control_fd);
         }
      } else if (COMMAND("SYST")) {
         fprintf(control_fd, "215 UNIX Type: L8.\n");
         fflush(control_fd);
      } else if (COMMAND("TYPE")) {
         fscanf(control_fd, "%s", arg);
         fprintf(control_fd, "200 OK\n");
         fflush(control_fd);
      } else if (COMMAND("QUIT")) {
         fprintf(control_fd, "221 Service closing control connection. Logged out if appropriate.\n");
         fflush(control_fd);
         close(data_socket);
         stop_server = true;
         break;
      } else {
         fprintf(control_fd, "502 Command not implemented.\n");
         fflush(control_fd);
         printf("Comando : %s %s\n", command, arg);
         fflush(control_fd);
         printf("Error interno del servidor\n");
         fflush(control_fd);
      }
   }

   fclose(control_fd);
};
