#ifndef AES_GCM_H
#define AES_GCM_H

#include <vector>
#include <cstdint>

class AesGcm {
  private:
    std::vector<uint8_t> key;
    uint8_t session_salt[4]; // 4bytes of randomness
    uint8_t counter;        //  8bytes of counting

  public:
    //constructor which take a 32-bytes shared key as input
    AesGcm(const std::vector<uint8_t>& shared_key);

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext);

    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& payload);

};

#endif
