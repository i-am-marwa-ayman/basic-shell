# Wish Shell

## Overview

Wish is a simple Unix shell that supports basic command execution, path management, input redirection, and batch mode execution. This project is part of the Operating Systems: Three Easy Pieces (OSTEP) coursework, designed to help understand shell implementation and process management in Unix-like systems.

## Features

- Interactive mode and batch mode.
- Supports built-in commands: `exit`, `cd`, and `path`.
- Executes external commands using system paths.
- Handles input redirection (`>`).
- Supports running multiple commands concurrently (`&`).

## Compilation

To compile the shell, run:

```sh
gcc -o wish wish.c
```

## Usage

### Interactive Mode

Run the shell by executing:

```sh
./wish
```

It will display a prompt:

```sh
wish> 
```

You can enter commands like:

```sh
wish> ls -l
```

### Batch Mode

Run a batch script:

```sh
./wish batchfile.txt
```

where `batchfile.txt` contains commands.

### Built-in Commands

- `exit`: Terminates the shell.
  ```sh
  wish> exit
  ```
- `cd <directory>`: Changes the current directory.
  ```sh
  wish> cd /home/user
  ```
- `path <directories>`: Sets the executable search path.
  ```sh
  wish> path /bin /usr/bin
  ```

### Input Redirection

Redirect output to a file using `>`:

```sh
wish> ls > output.txt
```

### Concurrent Execution

Run multiple commands in parallel using `&`:

```sh
wish> ls & ls -al
```

## Code Overview

- **Command Parsing**: The shell tokenizes user input to extract commands and arguments.
- **Path Management**: Supports modifying executable search paths.
- **Process Execution**: Uses `fork()` and `execv()` to run commands.
- **Redirection Handling**: Redirects output to a file when `>` is used.
- **Error Handling**: Displays an error message when commands fail.

## Error Handling

If an error occurs, the shell prints:

```sh
An error has occurred
```

## To-Do

- Add support for piping (`|`).
- Make > support no space.

