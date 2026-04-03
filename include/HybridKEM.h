#ifndef HYBRIDKEM_H
#define HYBRIDKEM_H

#include <vector>
#include <string>
#include <oqs/oqs.h>

class HybridKEM {

  private:
    std::string kem_name;
    OQS_KEM *kem;
    uint8_t *public_key;
    uint8_t *secret_key;

  public:
    HybridKEM();
    ~HybridKEM();

    std::vector<uint8_t> generate_keypair();

    std::pair<std::vector<uint8_t>,std::vector<uint8_t>> encapsulate(const std::vector<uint8_t>& peer_pk);

    std::vector<uint8_t> decapsulate(const std::vector<uint8_t>& ciphertext);
};

#endif
