CC			= gcc
CFLAGS		= -Wall -Wextra -Iinclude/server -Iinclude/client

CLIENT_NAME = client
CLIENT_SRCS = $(addprefix source/client/, \
                  client.c)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

SERVER_NAME = server
SERVER_SRCS =   $(addprefix source/server/, \
                  server.c)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

all: $(SERVER_NAME) $(CLIENT_NAME)

$(SERVER_NAME): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_NAME) $(SERVER_OBJS) -lX11 -llz4

$(CLIENT_NAME): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_NAME) $(CLIENT_OBJS) -llz4

clean:
	$(RM) $(SERVER_OBJS) $(CLIENT_OBJS)
