#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void server_init(Server *server, int port)
{
  server->socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server->socket < 0)
  {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt");
    close(server->socket);
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port        = htons(port);

  if (bind(server->socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("bind");
    close(server->socket);
    exit(EXIT_FAILURE);
  }

  if (listen(server->socket, SOMAXCONN) < 0)
  {
    perror("listen");
    close(server->socket);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d\n", port);

  server->endpoints.endpoints = NULL;
  server->endpoints.count     = 0;
}

void server_endpoint(Server *server, const char *path, void (*handler)(Client *))
{
  Endpoint *new_endpoints =
    (Endpoint *)realloc(server->endpoints.endpoints, sizeof(Endpoint) * (server->endpoints.count + 1));
  if (!new_endpoints)
    return;
  server->endpoints.endpoints = new_endpoints;
  strncpy(server->endpoints.endpoints[server->endpoints.count].path,
          path,
          sizeof(server->endpoints.endpoints[server->endpoints.count].path) - 1);
  server->endpoints.endpoints[server->endpoints.count]
    .path[sizeof(server->endpoints.endpoints[server->endpoints.count].path) - 1] = '\0';
  server->endpoints.endpoints[server->endpoints.count].handler                   = handler;
  server->endpoints.count++;
}

void server_response(Client *client, int code, const char *response)
{
  char buffer[1024];
  int length = sprintf(buffer,
                       "HTTP/1.1 %d OK\r\n"
                       "Content-Length: %u\r\n"
                       "Content-Type: text/plain\r\n"
                       "\r\n"
                       "%s",
                       code,
                       (unsigned int)strlen(response),
                       response);
  send(client->socket, buffer, length, 0);
}

void server_cleanup(Server *server)
{
  if (server->endpoints.endpoints)
  {
    free(server->endpoints.endpoints);
    server->endpoints.endpoints = NULL;
    server->endpoints.count     = 0;
  }
  if (server->socket >= 0)
  {
    close(server->socket);
  }
}

static int match_path(const char *format, const char *path, Values *values)
{
  if (!values || !values->arr)
    return 0;
  values->count = 0;

  const char *f = format;
  const char *p = path;

  while (*f && *p)
  {
    if (*f == '%' && f[1])
    {
      if (f[1] == '%')
      {
        if (*p != '%')
          return 0;
        f += 2;
        p++;
        continue;
      }

      if (values->count >= values->capacity)
        return 0;

      f++;
      char *end = NULL;

      if (*f == 'd')
      {
        values->arr[values->count].type    = SERVER_VALUE_INT;
        values->arr[values->count].value.i = strtol(p, &end, 10);
        if (end == p)
          return 0;
      }
      else if (*f == 'f')
      {
        values->arr[values->count].type    = SERVER_VALUE_FLOAT;
        double temp                        = strtod(p, &end);
        values->arr[values->count].value.f = (float)temp;
        if (end == p)
          return 0;
      }
      else if (*f == 's')
      {
        values->arr[values->count].type = SERVER_VALUE_STRING;
        int len                         = 0;
        char next                       = (f[1] && f[1] != '%') ? f[1] : '/';
        int max_len                     = sizeof(values->arr[values->count].value.s) - 1;
        while (*p && *p != next && len < max_len)
        {
          values->arr[values->count].value.s[len++] = *p++;
        }
        values->arr[values->count].value.s[len] = '\0';
        if (len == 0)
          return 0;
        end = (char *)p;
      }
      else
        return 0;

      p = end;
      f++;
      values->count++;
    }
    else
    {
      if (*f != *p)
        return 0;
      f++;
      p++;
    }
  }

  return *f == '\0' && *p == '\0';
}

int server_run(Server *server)
{
  int ret = 0;
  Client client;

  while (1)
  {
    char buffer[1024];
    char method[16];
    char path[256];

    socklen_t addr_len = sizeof(client.address);
    client.socket      = accept(server->socket, (struct sockaddr *)&client.address, &addr_len);
    if (client.socket < 0)
    {
      perror("accept");
      ret = client.socket;
      break;
    }

    ssize_t received = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
    if (received < 0)
    {
      perror("recv");
      ret = received;
      break;
    }
    buffer[received] = '\0';

    sscanf(buffer, "%15s %255s", method, path);

    printf("Received request: %s %s\n", method, path);

    Value values[10];
    client.values.arr      = values;
    client.values.count    = 0;
    client.values.capacity = 10;

    int handled = 0;
    for (int i = 0; i < server->endpoints.count; i++)
    {
      if (strcmp(server->endpoints.endpoints[i].path, path) == 0)
      {
        server->endpoints.endpoints[i].handler(&client);
        handled = 1;
        break;
      }

      if (match_path(server->endpoints.endpoints[i].path, path, &client.values))
      {
        server->endpoints.endpoints[i].handler(&client);
        handled = 1;
        break;
      }
    }

    if (!handled)
    {
      server_response(&client, 404, "Not Found\n");
    }

    close(client.socket);
  }

  return ret;
}
