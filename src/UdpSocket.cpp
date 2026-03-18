#include "../include/UdpSocket.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

UdpSocket::UdpSocket(){
  //create a udp socket ipv4
  sock = socket(AF_INET,SOCK_DGRAM,0);
  if(sock<0){
    perror("Error creating UDP socket");
  }
}


UdpSocket::~UdpSocket(){
  if(sock>0) close(sock);
}

bool UdpSocket::bindPort(int port){
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;//listen on all interfaces (wifi,ethernet,localhost)
    addr.sin_port = htons(port);
    
    if(::bind(sock, (struct sockaddr *)&addr, sizeof(addr))<0){
       perror("Bind Failed");
       return false;
    }

    std::cout<<"UDP socket bound to port"<<port<<std::endl;
    return true;
}

void UdpSocket::setPeer(const std::string& ip, int port){
  memset(&peerAddr, 0, sizeof(peerAddr));
  peerAddr.sin_family = AF_INET;
  peerAddr.sin_port = htons(port);

  if(inet_pton(AF_INET,ip.c_str(),&peerAddr.sin_addr)<=0){
    std::cerr << "Invalid IP address provided"<<std::endl;
  }
}

void UdpSocket::send(const char *data,size_t size){
  //send packet to the peer
  sendto(sock, data, size, 0, (struct sockaddr*)&peerAddr,sizeof(peerAddr));
}

int UdpSocket::receive(char *buffer, size_t size){
  struct sockaddr_in sender;
  socklen_t len = sizeof(sender);
  
  //receive from anyone and store who send it in 'sender'
  int n = recvfrom(sock, buffer,size,0,(struct sockaddr*)&sender,&len);

  //optional peerAddr can be updated here
  return n;
}

