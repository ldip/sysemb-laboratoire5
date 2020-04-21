CC			= clang
CFLAGS		= -Wall -Wextra -Iinclude/server

SERVER_NAME = server
SERVER_SRCS =   $(addprefix source/server/, \
                  server.c)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

all: $(SERVER_NAME)

$(SERVER_NAME): $(SERVER_OBJS)
	clang $(CFLAGS) -o $(SERVER_NAME) $(SERVER_OBJS) -lX11 -llz4

clean:
	$(RM) $(SERVER_OBJS)
