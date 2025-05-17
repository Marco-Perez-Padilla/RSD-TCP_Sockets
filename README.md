# Implementing a simple FTP server 


## Structure
The directory `ftp_server_cpp` contains a CMAKE C++ project. The other directory `ftp_server_testing` are for automatic testing that can be executed to check the implementation is correct

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

## Testing
1. Install and activate the python environment
```shell
sudo apt install python3.12-venv
sudo apt install python3-pip
python3 -m venv venv
source venv/bin/activate
pip install pytest pytest-asyncio
pip install pytest-timeout
```
2. Execute the tests. Important, do this only after compiling the code.
```shell
venv/bin/pytest
```
