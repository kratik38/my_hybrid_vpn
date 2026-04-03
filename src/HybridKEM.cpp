#include "../include/HybridKEM.h"
#include <stdexcept>
#include <iostream>

// setting up constructor for HybridKEM
HybridKEM::HybridKEM(){
    kem_name = OQS_KEM_alg_ml_kem_768;

//these functions are present in the #include oqs library which is attached in the HybridKEM.h
    kem = OQS_KEM_new(kem_name.c_str());
    if(kem == NULL){
      throw std:: runtime_error("FATAL: Failed to initialize ML-KEM-768. Check if liboqs is properly installed or not.");
    }

    public_key = new uint8_t[kem->length_public_key];
    secret_key = new uint8_t[kem->length_secret_key];
}


// setting up the destructor for hybridKEM
HybridKEM::~HybridKEM(){
  if(kem){
    OQS_KEM_free(kem);
  }
  delete[] public_key;
  delete[] secret_key;
}

// server generating the public key
// uint8_t is 8 bit in size and platform and archetecture independent
std::vector<uint8_t> HybridKEM::generate_keypair(){
  if(OQS_KEM_keypair(kem,public_key,secret_key) != OQS_SUCCESS){
    throw std::runtime_error("keypair generation failed");
  }

//convert raw c array into a safe modern c++ vector to send over the network
  return std::vector<uint8_t>(public_key,public_key + kem->length_public_key);
}

// client locks a secret using the server public key
std::pair<std::vector<uint8_t>,std::vector<uint8_t>> HybridKEM::encapsulate(const std::vector<uint8_t>& peer_pk){
  //temporary memoory to hold outputs
  uint8_t *ciphertext = new uint8_t[kem->length_ciphertext];
  uint8_t *shared_secret = new uint8_t[kem->length_shared_secret];
  

  if(OQS_KEM_encaps(kem,ciphertext,shared_secret,peer_pk.data()) != OQS_SUCCESS){
    delete[] ciphertext;
    delete[] shared_secret;
    throw std::runtime_error("Encapsulation failed");
  }

  std::vector<uint8_t> ct(ciphertext,ciphertext + kem->length_ciphertext);
  std::vector<uint8_t> ss(shared_secret,shared_secret + kem->length_shared_secret);


  //cleaning temp storage
  delete[] ciphertext;  
  delete[] shared_secret; 

  return {ss,ct};

}

std::vector<uint8_t> HybridKEM::decapsulate(const std::vector<uint8_t>& ciphertext){

  uint8_t *shared_secret = new uint8_t[kem->length_shared_secret];

  if(OQS_KEM_decaps(kem,shared_secret,ciphertext.data(),secret_key) != OQS_SUCCESS){
    delete[] shared_secret;
    throw std::runtime_error("Decapsulation failed");
  }

  std::vector<uint8_t> ss(shared_secret,shared_secret+kem->length_shared_secret);

  delete[] shared_secret;

  return ss;
}
