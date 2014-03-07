#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <ev.h>

static void read_cb(EV_P_ ev_io *w, int revents)
{
    char buffer[1024];
    int32_t count = ::read(w->fd, buffer, 1024);
    ::printf("%d\n", count);
    if(count == -1){
        ev_io_stop(EV_A_ w);
        return;
        //delete w;
    }
    ::printf("recv:%s\n", buffer);
    ::write(w->fd, buffer, count);
}


static void listen_cb(struct ev_loop* loop, ev_io *w, int revents)
{
    ::puts("new connection");
    ev_io* client_watcher = new ev_io;
    int32_t client_fd = ::accept(w->fd, (struct sockaddr*)NULL, NULL);
    ev_io_init(client_watcher, read_cb, client_fd, EV_READ);
    ev_io_start(loop, client_watcher);
    ev_unref(loop);
}


int main()
{
    struct ev_loop* loop = EV_DEFAULT;
    ev_io listen_watcher;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    int32_t listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    ::bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    ::listen(listen_fd, 10);

    ev_io_init(&listen_watcher, listen_cb, listen_fd, EV_READ);
    ev_io_start(loop, &listen_watcher);
    ev_run(loop, 0);
    return 0;
}
