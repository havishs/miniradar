#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <cstdlib>

static std::mutex clients_mu;
static std::vector<int> clients;     // open /events connections

static uint8_t crc8(const uint8_t *d, int n)
{
    uint8_t crc = 0;
    for (int i = 0; i < n; i++) {
        crc ^= d[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
    return crc;
}

// send one SSE message to every connected browser, drop dead connections
static void broadcast(const std::string &msg)
{
    std::lock_guard<std::mutex> lk(clients_mu);
    for (size_t i = 0; i < clients.size();) {
        if (write(clients[i], msg.data(), msg.size()) < 0) {
            close(clients[i]);
            clients.erase(clients.begin() + i);
        } else {
            i++;
        }
    }
}

static void handle_client(int cfd)
{
    char req[2048];
    ssize_t n = read(cfd, req, sizeof(req) - 1);
    if (n <= 0) { close(cfd); return; }
    req[n] = 0;

    if (strncmp(req, "GET /events", 11) == 0) {
        const char *h = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/event-stream\r\n"
                        "Cache-Control: no-cache\r\n"
                        "Connection: keep-alive\r\n\r\n";
        write(cfd, h, strlen(h));
        std::lock_guard<std::mutex> lk(clients_mu);
        clients.push_back(cfd);               // keep it open
    } else {
        std::ifstream f("radar.html");
        std::stringstream ss;
        ss << f.rdbuf();
        std::string body = ss.str();
        std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
                           "Content-Length: " + std::to_string(body.size()) +
                           "\r\nConnection: close\r\n\r\n" + body;
        write(cfd, resp.data(), resp.size());
        close(cfd);
    }
}

static void server_thread(int port)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // localhost only
    addr.sin_port = htons(port);
    if (bind(s, (sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); return; }
    listen(s, 8);

    while (true) {
        int cfd = accept(s, nullptr, nullptr);
        if (cfd >= 0) handle_client(cfd);
    }
}

enum State { SYNC1, SYNC2, BODY };

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s /dev/cu.xxx\n", argv[0]);
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);    // a closed browser tab must not kill us

    int fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0) { perror("open"); return 1; }

    termios t;
    tcgetattr(fd, &t);
    cfmakeraw(&t);
    speed_t spd = (argc > 2 && atoi(argv[2]) == 9600) ? B9600 : B115200;
    cfsetspeed(&t, spd);
    t.c_cflag |= (CLOCAL | CREAD);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &t) < 0) perror("tcsetattr");

    std::thread(server_thread, 8080).detach();
    printf("open http://localhost:8080 in your browser\n");

    State state = SYNC1;
    uint8_t body[6];
    int idx = 0;
    bool have_seq = false;
    uint8_t last_seq = 0;
    unsigned ok = 0, crc_err = 0, lost = 0;

    uint8_t b;
    while (read(fd, &b, 1) == 1) {
        switch (state) {
        case SYNC1:
            if (b == 0xAA) state = SYNC2;
            break;
        case SYNC2:
            if (b == 0x55) { state = BODY; idx = 0; }
            else if (b != 0xAA) state = SYNC1;
            break;
        case BODY:
            body[idx++] = b;
            if (idx == 6) {
                state = SYNC1;
                if (crc8(body, 4) == body[4]) {
                    uint8_t seq = body[0];
                    if (have_seq) lost += (uint8_t)(seq - last_seq - 1);
                    have_seq = true;
                    last_seq = seq;
                    ok++;
                    unsigned mm = body[2] | (body[3] << 8);
                    printf("seq=%3u angle=%3u mm=%4u ok=%u crc_err=%u lost=%u\n",
                           seq, body[1], mm, ok, crc_err, lost);
                    fflush(stdout);
                    char js[64];
                    snprintf(js, sizeof(js), "data: {\"a\":%u,\"mm\":%u}\n\n", body[1], mm);
                    broadcast(js);
                } else {
                    crc_err++;
                }
            }
            break;
        }
    }
    return 0;
}