#include "../include/HybridKEM.h"
#include <iostream>

// helper function to print the hex data
void print_hex(const std::string& label, const std::vector<uint8_t>& data){
  std::cout<<label<<" (first 16 bytes): ";
  for(int i=0;i<16 && i<data.size();i++){
    printf("%02x",data[i]);
  }
  std::cout<<"..."<<std::endl;
}

int main(){
  std::cout<<"--- Testing POST QUANTUM kyber-768 ----"<<std::endl;

  try{
    //1. seting up the 2 "computers"
    HybridKEM server;
    HybridKEM client;

    //2. server: generates public key
    std::cout<< "[server] Generating public key ..."<<std::endl;
    std::vector<uint8_t> server_pk = server.generate_keypair();
    print_hex("server public key",server_pk);
    std::cout<<std::endl;

    //3. client: receives public key encrypts the secret with that public key
    std::cout<< "[client] creating secret and locking it with server public key ..."<<std::endl;
    auto [client_secret,ciphertext] = client.encapsulate(server_pk);
    print_hex("client's shared_secret",client_secret);
    print_hex("ciphertext generated",ciphertext);
    std::cout<<std::endl;

    //4. server: receiving ciphertext, unlock it with server secret key
    std::cout<<"[server] received locked box/ciphertext unlocking..."<<std::endl;
    //server object already have the secret key (private val) which this function uses
    std::vector<uint8_t> server_secret = server.decapsulate(ciphertext);
    print_hex("server's shared secret",server_secret);
    std::cout<<std::endl;

    //5.final check
    
    if(client_secret == server_secret){
      std::cout << "success! both the secrets are exactly same "<<std::endl;
    }
    else{
      std::cout<<"failed! secrets do not match "<<std::endl;
    }
  }
  catch(const std::exception& e){
    std::cerr<<"Error: "<<e.what()<<std::endl;
  }

  return 0;
}
