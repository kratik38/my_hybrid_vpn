#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <string>
#include <netinet/in.h>

class UdpSocket{

  private:
    int sock;
    struct sockaddr_in peerAddr;// stores IP and Port of the other computer

  public:
    UdpSocket();
    ~UdpSocket();

    bool bindPort(int port);

    void setPeer(const std::string &ip,int port);

    void send(const char *data,size_t size);

    int receive(char *buffer,size_t size);

    int getFd() const{ return sock; }
};

#endif
