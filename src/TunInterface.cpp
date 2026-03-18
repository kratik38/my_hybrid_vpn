#include "../include/TunInterface.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdexcept>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

//os - specific headers

#if defined(__APPLE__)
    #include <sys/kern_control.h>
    #include <sys/sys_domain.h>
    #include <net/if_utun.h>
#elif defined(__linux__)
    #include <linux/if_tun.h> // part of linux kernal's user-space api(uapi)
    #include <net/if.h>
#endif

TunInterface::TunInterface(): fd(-1){
  memset(ifName,0,sizeof(ifName));
}

TunInterface::~TunInterface(){
  if(fd>0) close(fd);
}

int TunInterface::allocate(std::string devName){
  (void) devName;

  // macos implementation
  #if defined(__APPLE__)
   fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
   if(fd<0) return -1;

   // exist in apple's kernal control api(PF_SYSTEM)
  //To open a native VPN tunnel on macOS without installing third-party 
  //drivers (like TAP-Windows), you have to ask the kernel's "User Tunnel" 
  //controller to create one for you
  
   struct ctl_info ci;//similar to a DNS lookup - this phonebook lookup 
                      //shows id number for com.apple.net.utun_control 
  
   // struct ctl_info {
   // u_int32_t ctl_id;
   // char ctl)name[96];
   // }
   
   memset(&ci,0,sizeof(ci));
   strncpy(ci.ctl_name, UTUN_CONTROL_NAME, sizeof(ci.ctl_name));

   if(ioctl(fd, CTLIOCGINFO, &ci)==-1){
     close(fd);
     return -1;
   }
  
   struct sockaddr_ctl sc;// the mailing address
  //once we have the id we can use connect() to that id

  //struct sockaddr_ctl{
  //u_char sc_len; length of the struct standard socket
  //u_char sc_family; AF_SYSTEM (we are able to talk to system)
  //u_int16_t ss_sysaddr; AF_SYS_CONTROL (we are using kernal control)
  //u_int32_t sc_id; [INPUT] the id you got from ctl_info
  //u_int32_t sc_unit; [INPUT] which tunne; number 0 will give any 
  //  safe to use sc_unit = 0 so that kernal can any free tunnel for use
  //u_int32_t sc_reserved[5]; Padding ignore
  //};
  
  memset(&sc,0,sizeof(sc));
  sc.sc_id = ci.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = 0;

  if(connect(fd, (struct sockaddr *)&sc,sizeof(sc))==-1){
    close(fd);
    return -1;
  }

  //get the name of the tunnel
  socklen_t len = sizeof(ifName);

  //get socket options
  getsockopt(fd  // the current socket
      ,SYSPROTO_CONTROL // the level "the system protocol"
      ,UTUN_OPT_IFNAME  // the flag interface name
      ,ifName   // the output to the flag interface name 
      ,&len);  // the size of the buffer
               
  

  // linux implementation

#elif defined(__linux__)
  //linux uses the /dev/net/tun file   dev means device
  
  fd = open("/dev/net/tun",O_RDWR);
  if(fd<0) 
    return -1;

  struct ifreq ifr;
  memset(&ifr,0,sizeof(ifr));

  // IIF_TUN = TUN device (IF packets), IFF_NO_PI = no packet info header
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

  //here the tun name is provided 
  if(!devName.empty()){
    strncpy(ifr.ifr_name,devName.c_str(),IFNAMSIZ);
  }

  if(ioctl(fd,TUNSETIFF, (void *)&ifr)<0){
    close(fd);
    return -1;
  }

  strncpy(ifName, ifr.ifr_name, IFNAMSIZ);
#endif
   
  return fd;
}

ssize_t TunInterface::readPacket(char *buffer, size_t size){
  return read(fd, buffer, size);
}

ssize_t TunInterface::writePacket(char *buffer, size_t size){
  return write(fd, buffer, size);
}

std::string TunInterface::getName() const {
  return std::string(ifName);
}

int TunInterface::getFd() const {
  return fd;
}

