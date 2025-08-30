#include <stdio.h>

#include "server.h"

void endpoint_hello(Client *client)
{
  server_response(client, 200, "Hello, World!\n");
}

void endpoint_value(Client *client)
{
  if (client->values.count <= 0)
  {
    server_response(client, 400, "Bad Request\n");
    return;
  }

  if (client->values.arr[0].type != SERVER_VALUE_INT)
  {
    server_response(client, 400, "Bad Request\n");
    return;
  }

  int val = client->values.arr[0].value.i * 2;
  char response[64];
  sprintf(response, "Value doubled: %d\n", val);
  server_response(client, 200, response);
}

int main(void)
{
  Server server;
  if (server_init(&server, 8000) < 0)
    return 1;
  server_endpoint(&server, "/hello", endpoint_hello);
  server_endpoint(&server, "/value/%d", endpoint_value);
  return server_run(&server);
}
