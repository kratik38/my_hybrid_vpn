#include "../include/TunInterface.h"
#include "../include/UdpSocket.h"
#include "../include/HybridKEM.h"
#include "../include/AesGcm.h"
#include <memory>
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

void printSecretSnippet(const std::vector<uint8_t>& secret){
  std::cout<<"secret established! [ ";

  for(size_t i=0;i<4 && i<secret.size();i++){
    std::cout<< std::hex<<std::setw(2)<<std::setfill('0')<<(int)secret[i]<<" ";
  }
  
  std::cout<<std::dec<<"... ]"<<std::endl;
}

int main(){
  std::cout<<" -- Hybrid Post-Quanum VPN (Phase 4: Encrypted Tunnel) --"<<std::endl;

  TunInterface tun;
  UdpSocket udp;
  HybridKEM kem;
  std::vector<uint8_t> vpn_shared_secret;

  // declaring the AES engine pointer but keeping it empty(null ptr)
  std::unique_ptr<AesGcm> aes_engine = NULL;

  if(tun.allocate()<0){

    std::cerr<<"Error:could not open TUN interface use sudo if not used."<<std::endl;
    return 1;
  }

  std::string mode;
  std::cout<<"Enter Mode (server/client): ";
  std::cin>>mode;


  char buf[2048]; // buffer for network reading
 
  // *****************************
  //  THE POST QUANTUM HANDSHAKE
  // *****************************

  if(mode == "server"){
    udp.bindPort(5000);
    std::cout<<"\n[Handshake] waiting for client to initiate ..."<<std::endl;

    //1. wait for client's "hello" ping
    udp.receive(buf,sizeof(buf));
    std::cout<<"[Handshake] client detected. Generating kyber keypair..."<<std::endl;

    //2. Generate and send public key (tag 1)
    auto pk = kem.generate_keypair();
    std::vector<uint8_t> msg;
    msg.push_back(1);//tag 1
    msg.insert(msg.end(),pk.begin(),pk.end());
    udp.send(reinterpret_cast<const char*>(msg.data()),msg.size());
    
    std::cout<<"[Handshake] public key send. waiting for ciphertext..."<<std::endl;
    
    //3. Receive ciphertext (tag 2) and decapsulate
    
    int n = udp.receive(buf,sizeof(buf));
    if(buf[0]==2){  // verify tag 2
        std::vector<uint8_t> ct(buf+1,buf+n);//extracting everything after the tag
        vpn_shared_secret = kem.decapsulate(ct);
        printSecretSnippet(vpn_shared_secret);

        // initializing server's AES engine with the secret
        aes_engine = std::make_unique<AesGcm>(vpn_shared_secret);
    }
  }
  else{
    std::string peerIP;
    std::cout<<"Starting as CLIENT."<<std::endl;
    std::cout<<"Enter Server IP (e.g 127.0.0.1): ";
    std::cin>> peerIP;
    udp.setPeer(peerIP,5000);
    udp.bindPort(0);

    //1. send hello ping to wake up the server 
    char ping =0;
    udp.send(&ping,1);
    std::cout<<"\n[Handshake] ping send. waiting for server's public key..."<<std::endl;

    //2. Receive public key (tag1), encapsulate, and save secret

    int n = udp.receive(buf,sizeof(buf));
    if(buf[0]==1){ //verify tag1
        std::vector<uint8_t> pk(buf+1,buf+n);
        auto [ss,ct] = kem.encapsulate(pk);
        vpn_shared_secret = ss; // save our copy of the secret
        
        //3. send ciphertext back (tag2)
        std::vector<uint8_t> msg;
        msg.push_back(2); //tag2
        msg.insert(msg.end(),ct.begin(),ct.end());
        udp.send(reinterpret_cast<const char*>(msg.data()),msg.size());

        printSecretSnippet(vpn_shared_secret);


        // initializing client's AES engine with the secret
        aes_engine = std::make_unique<AesGcm>(vpn_shared_secret);
    }
  }

  // ***********************
  // NORMAL TUNNEL OPERATION
  // ***********************
  
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
  //(tagging with 3)
  if(FD_ISSET(tun.getFd(),&readFDs)){
    ssize_t n = tun.readPacket(buf+1,sizeof(buf)-1);

    // replacing the old send logic with encryption
    if(n>0 && aes_engine){
        // taking raw os network packing
      std::vector<uint8_t> plaintext(buf,buf+n);

      //encrypting it
      std::vector<uint8_t> ciphertext = aes_engine->encrypt(plaintext);

      //prepend protocol tag(3) and send it over UDP
      std::vector<uint8_t> secure_packet;
      secure_packet.push_back(3);
      secure_packet.insert(secure_packet.end(),ciphertext.begin(),ciphertext.end());

      udp.send(reinterpret_cast<const char*>(secure_packet.data()),secure_packet.size());
    }

  }

  //case 2 Data arrived from network(UDP)-> write to kernal(TUN)
  if(FD_ISSET(udp.getFd(),&readFDs)){
    ssize_t n = udp.receive(buf,sizeof(buf));

    if(n>0 && buf[0] == 3 && aes_engine){
      //extract everything after tag3
      std::vector<uint8_t> ciphertext(buf+1,buf+n);

      try{
        //decrypting it
        std::vector<uint8_t> plaintext = aes_engine->decrypt(ciphertext);

        //inject the decrypted packet back into the local OS
        tun.writePacket(reinterpret_cast<char*>(plaintext.data()),plaintext.size());
      } 
       catch(const std::exception& e){
         std::cerr<<"\n[SECURITY WARNING] Dropped invalid packet: "<<e.what()<<std::endl;
       }
    }
  }

 }

return 0;

}
