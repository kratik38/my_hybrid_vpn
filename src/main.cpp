#include "../include/TunInterface.h"
#include <iostream>
#include <iomanip>
#include <vector>

void printHex(const char *buffer, ssize_t len){

  //usually prints first 20 bytes which is the ip header
  std::cout<<"[DATA "<<len<<"B]: ";
  for(int i=0;i<std::min((ssize_t)16,len);i++){
    std::cout<< std::hex<<std::setw(2)<<std::setfill('0')
       <<(unsigned int)(unsigned char)buffer[i]<<" ";
  }
  std::cout <<"..."<< std::dec << std::endl;
}

int main(){
  std::cout<<" -- Hybrid Post-Quanum VPN (Phase 1: TUNnel)--"<<std::endl;

  TunInterface tun;

  if(tun.allocate()<0){
    std::cerr<<"CRITICAL ERROR: Failed to open TUN device."<<std::endl;
    std::cerr<<"-> Are you running with sudo?"<<std::endl;
#ifdef __linux__
    std::cerr<<"-> Does /dev/net/tun exits?"<<std::endl;
#endif
    return 1;
  }

  std::cout<<"SUCCESS: Interface allocated: "<<tun.getName()<<std::endl;
  std::cout<<"Command to run (in new terminal):"<<std::endl;

#ifdef __APPLE__
  std::cout<<" sudo ifconfig "<<tun.getName()<<" 10.0.0.1 10.0.0.2 up"<<std::endl;
#elif __linux__
  std::cout<<" sudo ip addr add 10.0.0.1/24 dev "<<tun.getName()<<std::endl;
  std::cout<<" sudo ip link set up dev "<<tun.getName()<<std::endl;
#endif

std::cout<< "Listening for packets..."<<std::endl;

char buffer[2048];

while(true){
  ssize_t bytes = tun.readPacket(buffer,sizeof(buffer));
  if(bytes>0)
    printHex(buffer,bytes);
  }

return 0;

}
