[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/mP7qMjUr)
[![Open in Codespaces](https://classroom.github.com/assets/launch-codespace-2972f46106e565e64193e422d61a12cf1da4916b45550586e14ef0a7c637dd04.svg)](https://classroom.github.com/open-in-codespaces?assignment_repo_id=19092252)
# Implementing a simple FTP server 

This assigment is to be developed using C++ in linux. Students must complete this starter code so it implements the functions mentioned in the pdf dossier of this assignment.

## Structure
The only files that you have to edit are in the directory `ftp_server_cpp`. This directory contains a CMAKE C++ project. The other directory `ftp_server_testing` are for automatic testing for Github Classroom and you should not use them. 

## Compiling the code
1. Change to the root of the CPP project: 
```shell 
cd ftp_server_cpp
```
2. Generate:
```shell 
cmake .
```
3. Run make to start the compilation 
```shell 
make
```

After this last command, a binary named `ftp_server` should have been generated in this directory.

## Running
```shell
cd ftp 
./ftp_server [port]
```
Note that the `port` argument is optional. A random one will be assigned if not specified.
