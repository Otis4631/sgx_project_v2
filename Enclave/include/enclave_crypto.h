#include <memory>

using namespace std;


enum CIPHER {
    RC4, SALSA20, AES128
};
class Crypto {
    public:
        Crypto() = default;
        Crypto(uint8_t* _public_key, int pk_len, int _sym_key_len, CIPHER type);

    private:
        unique_ptr<uint8_t> sym_key;
        int sym_key_len;

        unique_ptr<uint8_t> public_key;
        int public_key_len;

        unique_ptr<uint8_t> sym_key_encrypted;
        int sym_key_encrypted_len;

        CIPHER cipher_type;
};