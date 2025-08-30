# SimpleHTTP

Lightweight single-file C99 HTTP server library for fast integration and endpoint-based code execution.

## Features

- URL pattern matching (`/api/%d`, `/user/%s/%d`)
- Parameter extraction (int, float, string)
- Static and dynamic linking support

## Usage

```c
#include "server.h"

void hello_handler(Client *client) {
    server_response(client, 200, "Hello, World!\n");
}

int main() {
    Server server;
    server_init(&server, 8000);
    server_endpoint(&server, "/hello", hello_handler);
    server_endpoint(&server, "/value/%d", value_handler);
    return server_run(&server);
}
```

## Build

**Static:**
```bash
gcc example.c server.c -o example
```

**Dynamic:**
```bash
gcc -fPIC -shared -o libserver.so server.c
gcc -Wl,-rpath,'$ORIGIN' example.c -L. -lserver -o example
```
