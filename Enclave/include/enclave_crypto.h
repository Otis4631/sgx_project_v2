#include <memory>

using namespace std;


enum CIPHER {
    RC4, SALSA20, AES128GCM
};
class Crypto {
    public:
        Crypto() = default;
        Crypto(uint8_t* _public_key, int pk_len, int _sym_key_len, int iv_len, CIPHER type);
        ~Crypto();

        int encrypt(uint8_t* out, size_t* out_len, uint8_t* in, size_t in_len);
        int decrypt(uint8_t* out, size_t* out_len, uint8_t* in, size_t in_len);

        int gen_sym_key();

        uint8_t* sym_key = NULL;
        int sym_key_len;

        uint8_t* public_key = NULL;
        int public_key_len;

        uint8_t* sym_key_encrypted = NULL;
        int sym_key_encrypted_len;

        uint8_t* iv = NULL;
        int iv_len;

        CIPHER cipher_type;
};