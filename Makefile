CC = gcc
CFLAGS = -Wall -O2 -g
LIBS = -lvncserver

SRCS = vnc-server.c sud-interceptor.c framebuffer-redirector.c
OBJS = $(SRCS:.c=.o)
EXEC = framebuffer-server

$(EXEC): $(OBJS)
    $(CC) $(OBJS) -o $(EXEC) $(LIBS)

clean:
    rm -f $(OBJS) $(EXEC)
