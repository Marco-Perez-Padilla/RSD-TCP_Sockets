import asyncio
import filecmp
import os
import shutil
from ftplib import FTP

import pytest

SERVER_DIR = None
CLIENT_DIR = None
executable_path = "./ftp_server_cpp/ftp_server"


def run_server(port=0):
    process = asyncio.subprocess.create_subprocess_shell(
        f'cd {SERVER_DIR} && ./ftp_server {port}',
        stdin=asyncio.subprocess.PIPE,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE
    )
    return process


@pytest.fixture(scope="session")
def session_fixture(tmpdir_factory):
    # Setup

    # Create a directory containing a test file for server and client
    global SERVER_DIR
    global CLIENT_DIR
    SERVER_DIR = str(tmpdir_factory.mktemp("server"))
    CLIENT_DIR = str(tmpdir_factory.mktemp("client"))

    # Copy binary to this path
    shutil.copy(executable_path, SERVER_DIR)

    # Create a file of 250 MB
    file_size_in_bytes = 1024 * 1024 * 250
    test_file_path = f'{CLIENT_DIR}/test_file_client.bin'
    with open(test_file_path, 'wb') as fout:
        fout.write(os.urandom(file_size_in_bytes))
    # Create a file of 128 MB
    file_size_in_bytes = 1024 * 1024 * 128
    test_file_path = f'{SERVER_DIR}/test_file_server.bin'
    with open(test_file_path, 'wb') as fout:
        fout.write(os.urandom(file_size_in_bytes))

    os.chdir(CLIENT_DIR)

    yield

    # Teardown
    shutil.rmtree(f'{SERVER_DIR}')
    shutil.rmtree(f'{CLIENT_DIR}')


@pytest.mark.asyncio
async def test_run_server(session_fixture):
    process = await run_server()
    recovered_string = await process.stdout.readline()
    recovered_string = recovered_string.decode()
    assert 'Listening' in recovered_string
    process.terminate()
    await process.wait()


@pytest.mark.asyncio
async def test_active_mode_get(session_fixture):
    """ Getting a file in active mode """
    process = await run_server()
    recovered_string = await process.stdout.readline()
    recovered_string = recovered_string.decode()

    assert 'Listening' in recovered_string
    port = int(recovered_string.split(' ')[-1])

    # Create client and login
    ftp = FTP()
    ftp.connect('127.0.0.1', port)
    ret = ftp.login('test', '1234')
    assert '230' in ret

    ftp.set_pasv(False)

    input_file = f'{SERVER_DIR}/test_file_server.bin'
    output_file = f'transferred_test_file_server.bin'

    with open(output_file, 'wb') as fp:
        ret = ftp.retrbinary(f'RETR {input_file}', fp.write)
        assert '226' in ret
        fp.close()
    # Check that the transferred file is correct
    assert filecmp.cmp(input_file, output_file)

    # Cleanup the file
    os.remove(output_file)

    ftp.close()
    process.terminate()
    await process.wait()


@pytest.mark.asyncio
async def test_passive_mode_get(session_fixture):
    """ Getting a file in passive mode """
    process = await run_server()
    recovered_string = await process.stdout.readline()
    recovered_string = recovered_string.decode()

    assert 'Listening' in recovered_string
    port = int(recovered_string.split(' ')[-1])

    # Create client and login
    ftp = FTP()
    ftp.connect('127.0.0.1', port)
    ret = ftp.login('test', '1234')
    assert '230' in ret

    ftp.set_pasv(True)

    input_file = f'{SERVER_DIR}/test_file_server.bin'
    output_file = f'transferred_test_file_server.bin'

    with open(output_file, 'wb') as fp:
        ret = ftp.retrbinary(f'RETR {input_file}', fp.write)
        assert '226' in ret
        fp.close()
    # Check that the transferred file is correct
    assert filecmp.cmp(input_file, output_file)

    # Cleanup the file
    os.remove(output_file)

    ftp.close()
    process.terminate()
    await process.wait()


@pytest.mark.asyncio
async def test_active_mode_put(session_fixture):
    """ Putting a file in active mode """
    process = await run_server()
    recovered_string = await process.stdout.readline()
    recovered_string = recovered_string.decode()

    assert 'Listening' in recovered_string
    port = int(recovered_string.split(' ')[-1])

    # Create client and login
    ftp = FTP()
    ftp.connect('127.0.0.1', port)
    ret = ftp.login('test', '1234')
    assert '230' in ret

    ftp.set_pasv(False)

    input_file = f'test_file_client.bin'
    output_file = f'{SERVER_DIR}/{input_file}'

    with open(input_file, 'rb') as fp:
        ret = ftp.storbinary(f'STOR {input_file}', fp)
        assert '226' in ret
        fp.close()

    # Check that the transferred file is correct
    assert filecmp.cmp(input_file, output_file)
    ftp.close()
    process.terminate()
    await process.wait()


@pytest.mark.asyncio
async def test_passive_mode_put(session_fixture):
    """ Putting a file in active mode """
    process = await run_server()
    recovered_string = await process.stdout.readline()
    recovered_string = recovered_string.decode()

    assert 'Listening' in recovered_string
    port = int(recovered_string.split(' ')[-1])

    # Create client and login
    ftp = FTP()
    ftp.connect('127.0.0.1', port)
    ret = ftp.login('test', '1234')
    assert '230' in ret

    ftp.set_pasv(True)

    input_file = f'test_file_client.bin'
    output_file = f'{SERVER_DIR}/{input_file}'

    with open(input_file, 'rb') as fp:
        ret = ftp.storbinary(f'STOR {input_file}', fp)
        assert '226' in ret
        fp.close()

    # Check that the transferred file is correct
    assert filecmp.cmp(input_file, output_file)
    ftp.close()
    process.terminate()
    await process.wait()


@pytest.mark.asyncio
async def test_active_mode_list(session_fixture):
    """ List in active mode """
    process = await run_server()
    recovered_string = await process.stdout.readline()
    recovered_string = recovered_string.decode()

    assert 'Listening' in recovered_string
    port = int(recovered_string.split(' ')[-1])

    # Create client and login
    ftp = FTP()
    ftp.connect('127.0.0.1', port)
    ret = ftp.login('test', '1234')
    assert '230' in ret

    ftp.set_pasv(False)

    dir_data = []

    def callback(line):
        dir_data.append(line)

    ftp.dir('.', callback)

    # There should be two files in the dir's output:
    # -> 'test_file_server.bin'
    # -> 'ftp_server.bin'

    assert 'test_file_server.bin' in dir_data
    assert 'ftp_server' in dir_data

    ftp.close()
    process.terminate()
    await process.wait()


@pytest.mark.asyncio
async def test_passive_mode_list(session_fixture):
    """ List in active mode """
    process = await run_server()
    recovered_string = await process.stdout.readline()
    recovered_string = recovered_string.decode()

    assert 'Listening' in recovered_string
    port = int(recovered_string.split(' ')[-1])

    # Create client and login
    ftp = FTP()
    ftp.connect('127.0.0.1', port)
    ret = ftp.login('test', '1234')
    assert '230' in ret

    ftp.set_pasv(True)

    dir_data = []

    def callback(line):
        dir_data.append(line)

    ftp.dir('.', callback)

    # There should be two files in the dir's output:
    # -> 'test_file_server.bin'
    # -> 'ftp_server.bin'

    assert 'test_file_server.bin' in dir_data
    assert 'ftp_server' in dir_data

    ftp.close()
    process.terminate()
    await process.wait()


@pytest.mark.asyncio
async def test_graceful_shutdown(session_fixture):
    for i in range(10):
        process = await run_server(2121)
        recovered_string = await process.stdout.readline()
        recovered_string = recovered_string.decode()
        assert 'Listening' in recovered_string
        port = int(recovered_string.split(' ')[-1])
        assert port == 2121
        process.terminate()
        await process.wait() # This one is problematic (just on github)

@pytest.mark.asyncio
async def test_put_get_list_active_passive(session_fixture):
    process = await run_server()
    recovered_string = await process.stdout.readline()
    recovered_string = recovered_string.decode()

    assert 'Listening' in recovered_string
    port = int(recovered_string.split(' ')[-1])

    # Create client and login
    ftp = FTP()
    ftp.connect('127.0.0.1', port)
    ret = ftp.login('test', '1234')
    assert '230' in ret

    # Put a file in passive mode
    ftp.set_pasv(True)

    input_file = f'test_file_client.bin'
    output_file = f'{SERVER_DIR}/{input_file}'

    with open(input_file, 'rb') as fp:
        ret = ftp.storbinary(f'STOR {input_file}', fp)
        assert '226' in ret
        fp.close()

    # Check that the transferred file is correct
    assert filecmp.cmp(input_file, output_file)

    # Get a file in active mode
    ftp.set_pasv(False)

    input_file = f'{SERVER_DIR}/{input_file}'
    output_file = f'transferred_test_file_server.bin'

    with open(output_file, 'wb') as fp:
        ret = ftp.retrbinary(f'RETR {input_file}', fp.write)
        assert '226' in ret
        fp.close()
    # Check that the transferred file is correct
    assert filecmp.cmp(input_file, output_file)

    # List a file in passive mode
    ftp.set_pasv(True)

    dir_data = []

    def callback(line):
        dir_data.append(line)

    ftp.dir('.', callback)

    # There should be two files in the dir's output:
    # -> 'test_file_server.bin'
    # -> 'ftp_server.bin'

    assert 'test_file_server.bin' in dir_data
    assert 'ftp_server' in dir_data

    # Put a file in active mode
    ftp.set_pasv(False)

    input_file = f'test_file_client.bin'
    output_file = f'{SERVER_DIR}/{input_file}'

    with open(input_file, 'rb') as fp:
        ret = ftp.storbinary(f'STOR {input_file}', fp)
        assert '226' in ret
        fp.close()

    # Check that the transferred file is correct
    assert filecmp.cmp(input_file, output_file)

    # Get a file in active mode
    ftp.set_pasv(True)

    input_file = f'{SERVER_DIR}/{input_file}'
    output_file = f'transferred_test_file_server.bin'

    with open(output_file, 'wb') as fp:
        ret = ftp.retrbinary(f'RETR {input_file}', fp.write)
        assert '226' in ret
        fp.close()
    # Check that the transferred file is correct
    assert filecmp.cmp(input_file, output_file)

    # List a file in active mode
    ftp.set_pasv(False)

    dir_data = []

    def callback(line):
        dir_data.append(line)

    ftp.dir('.', callback)

    # There should be two files in the dir's output:
    # -> 'test_file_server.bin'
    # -> 'ftp_server.bin'

    assert 'test_file_server.bin' in dir_data
    assert 'ftp_server' in dir_data

    process.terminate()
    # await process.wait() # This one is problematic (just on github)
