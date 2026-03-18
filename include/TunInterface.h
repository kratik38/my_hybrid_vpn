#ifndef TUNINTERFACE_H
#define TUNINTERFACE_H

#include <string>
#include <vector>

class TunInterface{

  private:
    int fd; //file descriptor stores the tunnel id 
    char ifName[32]; //stores tunnel name ex. utun2 in mac or tun0 in linux

  public:
    TunInterface();
    ~TunInterface();

    //function which behaves differntly on mac vs linux
    int allocate(std::string devName = "");

    //core io functions
    ssize_t readPacket(char *buffer,size_t size);
    ssize_t writePacket(char *buffer,size_t size);

    std::string getName() const;
    int getFd() const;

};

#endif
