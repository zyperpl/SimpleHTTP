#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <sys/socket.h>

#ifdef _WIN32
#ifdef SERVER_EXPORTS
#define SERVER_API __declspec(dllexport)
#else
#define SERVER_API __declspec(dllimport)
#endif
#else
#ifdef SERVER_EXPORTS
#define SERVER_API __attribute__((visibility("default")))
#else
#define SERVER_API
#endif
#endif

typedef enum
{
  SERVER_VALUE_INT,
  SERVER_VALUE_FLOAT,
  SERVER_VALUE_STRING
} ServerValueType;

typedef struct
{
  ServerValueType type;
  union
  {
    int i;
    float f;
    char s[256];
  } value;
} Value;

typedef struct
{
  Value *arr;
  int count;
  int capacity;
} Values;

typedef struct Client
{
  int socket;
  struct sockaddr_in address;
  Values values;
} Client;

typedef struct Endpoint
{
  char path[256];
  void (*handler)(Client *);
} Endpoint;

typedef struct ServerEndpoints
{
  Endpoint *endpoints;
  int count;
} ServerEndpoints;

typedef struct Server
{
  int socket;
  ServerEndpoints endpoints;
} Server;

SERVER_API void server_init(Server *server, int port);
SERVER_API void server_endpoint(Server *server, const char *path, void (*handler)(Client *));
SERVER_API void server_response(Client *client, int code, const char *response);
SERVER_API void server_cleanup(Server *server);
SERVER_API int server_run(Server *server);

#endif
