#include "../include/AesGcm.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <iostream>
#include <cstring>

const int IV_LEN = 12;
const int TAG_LEN = 16; // GCM tag used for authentication

AesGcm::AesGcm(const std::vector<uint8_t>& shared_key){
  if(shared_key.size() != 32){
    throw std::invalid_argument("AES-256 require exactly a 32-byte key.");
  }

  key = shared_key;

  // generates 4 byte random salt once per tunnel session
  if(RAND_bytes(session_salt,4) != 1){
    throw std::runtime_error("Failed to generate secure random salt.");
  }

  counter = 0;
}

std::vector<uint8_t> AesGcm::encrypt(const std::vector<uint8_t>& plaintext){
    std::vector<uint8_t> iv(IV_LEN);

    //build the 12 byte iv having 4 bute salt and 8 by counter
    std::memcpy(iv.data(),session_salt,4);
    std::memcpy(iv.data()+4,&counter,sizeof(counter));

    //incrementing the counter for next packet
    counter++;

    std::vector<uint8_t> ciphertext(plaintext.size());
    std::vector<uint8_t> tag(TAG_LEN);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;
    int ciphertext_len;

    //initialize AES-256-GCM
    EVP_EncryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,NULL,NULL);
    EVP_EncryptInit_ex(ctx,NULL,NULL,key.data(),iv.data());

    //Encrypt the data
    EVP_EncryptUpdate(ctx,ciphertext.data(),&len,plaintext.data(),plaintext.size());//stage 1:encryption 
    ciphertext_len = len;
 
    //finalize the encryption and generate authentication tag
    EVP_EncryptFinal_ex(ctx,ciphertext.data()+len,&len);//stage 2:cleanup
    ciphertext_len +=len;//ciphertext.data()+len is a pointer operation to start cleanup after that position
                         //len is size of the padding extra padding bytes of authenication after that cipher+len position
    EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_GET_TAG,TAG_LEN,tag.data());

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(ciphertext_len);

    //package it all togher [iv 12Byte |tag 16Byte |ciphertext]
    std::vector<uint8_t> payload;//package
    payload.insert(payload.end(),iv.begin(),iv.end());
    payload.insert(payload.end(),tag.begin(),tag.end());
    payload.insert(payload.end(),ciphertext.begin(),ciphertext.end());
    
    return payload;
}

std::vector<uint8_t> AesGcm::decrypt(const std::vector<uint8_t>& payload){
  if(payload.size() < IV_LEN + TAG_LEN){
    throw std::runtime_error("Payload too short to be valid AES-GCM data.");
  }
  std::vector<uint8_t> iv(payload.begin(),payload.begin()+ IV_LEN);
  std::vector<uint8_t> tag(payload.begin()+IV_LEN,payload.begin()+IV_LEN+TAG_LEN);
  std::vector<uint8_t> ciphertext(payload.begin()+IV_LEN+TAG_LEN,payload.end());
  std::vector<uint8_t> plaintext(ciphertext.size());


  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  int len;
  int plaintext_len;

  //initialize decryption
  EVP_DecryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,NULL,NULL);
  EVP_DecryptInit_ex(ctx,NULL,NULL,key.data(),iv.data());

  //decrypt the data
  EVP_DecryptUpdate(ctx,plaintext.data(),&len,ciphertext.data(),ciphertext.size());
  plaintext_len = len;


  //set the expected tag to verify authenticity before finalizing
  EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_TAG,TAG_LEN,tag.data());

  //finalize if this fails (return <=0) the data was tampered or corrupted or key is wrong
  int ret = EVP_DecryptFinal_ex(ctx,plaintext.data()+len,&len);
  EVP_CIPHER_CTX_free(ctx);

  if(ret>0){
    plaintext_len += len;
    plaintext.resize(plaintext_len);
    return plaintext;
  }
  else{
    throw std::runtime_error("AES-GCM decryption failed!(key is wrong or data is tampered or corrupted)");
  }
}

