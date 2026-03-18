#include "../include/TunInterface.h"
#include "../include/UdpSocket.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <sys/select.h> //require for handling of multiple inputs
                        //making tunnel and handling udpSocket

/* used in phase of phase 1
  void printHex(const char *buffer, ssize_t len){

  //usually prints first 20 bytes which is the ip header
  std::cout<<"[DATA "<<len<<"B]: ";
  for(int i=0;i<std::min((ssize_t)16,len);i++){
    std::cout<< std::hex<<std::setw(2)<<std::setfill('0')
       <<(unsigned int)(unsigned char)buffer[i]<<" ";
  }
  std::cout <<"..."<< std::dec << std::endl;
}*/

int getMaxFd(int fd1,int fd2){
  return (fd1>fd2)? fd1 : fd2;
}

int main(){
  std::cout<<" -- Hybrid Post-Quanum VPN (Phase 2) --"<<std::endl;

  TunInterface tun;
  UdpSocket udp;

  if(tun.allocate()<0){

    std::cerr<<"Error:could not open TUN interface use sudo if not used."<<std::endl;
    return 1;
  }

  std::string mode;
  std::cout<<"Enter Mode (server/client): ";
  std::cin>>mode;

  if(mode == "server"){
    std::cout<<"Starting as SERVER. Listening on UDP PORT 5000..."<<std::endl;
    udp.bindPort(5000);
  }
  else{
    std::string peerIP;
    std::cout<<"Starting as CLIENT."<<std::endl;
    std::cout<<"Enter Server IP (e.g 127.0.0.1): ";
    std::cin>> peerIP;
    udp.setPeer(peerIP,5000);
    udp.bindPort(0);
  }

  std::string tunName = tun.getName();
  std::cout<<"\n[!] Tunnel Created: "<<tunName<<std::endl;
  std::cout<<"[!] Run This in NEW TERMINAL:"<<std::endl;

#ifdef __APPLE__
  std::cout<<" sudo ifconfig "<<tunName<<" 10.0.0.1 10.0.0.2 up"<<std::endl;
#elif __linux__
  std::cout<<" sudo ip addr add 10.0.0.1/24 dev "<<tunName<<std::endl;
  std::cout<<" sudo ip link set up dev "<<tunName<<std::endl;
#endif

std::cout<< "\n -- Bridge is Running... (Press Ctrl+C to stop) --"<<std::endl;

// main part select multiplexer
char buffer[2048];
fd_set readFDs; // a list of file descriptors

while(true){
  //clearing the list and add our two sources(TUN and UDP)
  FD_ZERO(&readFDs);
  FD_SET(tun.getFd(),&readFDs);
  FD_SET(udp.getFd(),&readFDs);

  int max_fd = getMaxFd(tun.getFd(),udp.getFd());

  //wait until data arrrives on either interface
  int activity = select(max_fd + 1, &readFDs,NULL,NULL,NULL);

  if(activity<0){
    perror("select error");
    break;
  }

  //case 1 Data arrived from the kernal(TUN) -> send to UDP
  if(FD_ISSET(tun.getFd(),&readFDs)){
    ssize_t n = tun.readPacket(buffer,sizeof(buffer));
    if(n>0){
      udp.send(buffer,n);
      std::cout<<"-> Forwarded "<<n<<" bytes to UDP."<<std::endl;
    }
  }

  //case 2 Data arrived from network(UDP)-> write to kernal(TUN)
  if(FD_ISSET(udp.getFd(),&readFDs)){
    ssize_t n = udp.receive(buffer,sizeof(buffer));
    if(n>0){
      tun.writePacket(buffer,n);
      std::cout<<"<-Received "<<n<<" bytes from UDP."<<std::endl;
    }
  }
}

return 0;

}
