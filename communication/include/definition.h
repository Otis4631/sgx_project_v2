#pragma once

#define BUFF_SIZE          1024

#define MIN_VERSION           1

#define STAGE_INIT       0x0001
#define STAGE_CRYPT      0x0002

#define STAGE_UPLOAD     0x0004
#define STAGE_DOWNLOAD   0x0008
#define STAGE_WAITING    0x0010

#define STAGE_DESTORYED  0x0020

#define CRYPT_HEADER_SIZE       5
#define HELLO_HEADER_SIZE       2
/*
+-----------------------------------------------+
|       read one batch data                 |
|       stage_init(send auth, build connection) |
|       change stage to encrypt                 |
+-----------------------------------------------+
                    |
+-----------------------------------------------+
|       stage_encrypt(get sym key)              |
|       encrypt data batch by batch)            |
|       change stage to upload                  |
+-----------------------------------------------+   
                    |
+-----------------------------------------------+
|       stage_upload(upload one batch)          |
|       change stage to download                |
+-----------------------------------------------+   
                    |
+-------------------------------------------------------+
|       stage_download(wait for command)                |
|       change stage to download, upload, wait, destrory|
+-------------------------------------------------------+ 
                    \\
+-------------------------------------------------------+
|       stage_wait(keep alive until new command)        |
|       change stage to download, upload, wait, destrory|
+-------------------------------------------------------+  
                    \\
+-------------------------------------------------------+
|       stage_destory(destory client)                   |
+-------------------------------------------------------+  

Hello Format:
+-------+-------+--------+
|  VER  |  CMD  |  DATA  |
+-------+-------+--------+
|   1   |   1   |Variable|
+-------+-------+--------+
VER:    protocol version: 0x01
CMD:        
        0x01: client: send UID
        0x0f: response ping

        0x11: server: UID is ok
        0x12: server: UID is not premited
        0x1f: send ping

        


Crypt Exchange Format:
+-------+---------+-------+-------+-----------+-----------+
|  VER  |   CMD   | K_LEN | N_LEN |  IV_LEN   |    DATA   |
+-------+---------+-------+-------+-----------+-----------+
|   1   |    1    |   1   |   1   |     1     |  Variable |
+-------+---------+-------+-------+-----------+-----------+

VER:        protocol version: 0x01
CMD:
    0x01:   client: request for new key. 
    0x02:   client: check the key id uploaded in data if is available.
    
    0x11:   server:  send the new key.
    0x12:   server:  return the key id works.

size field denotes: x for 2^x, e.g. 0 for 2^0 = 1, 4 for 2^4 = 16
K_LEN:      for symmetirc key is key_len + iv_len, for public key is n_len + e_len.
N_LEN:      n len of public key, 0 for others, correspondingly, e_len is data_len - n_len. 
IV_LEN:     iv len of sym key, 0 for others, correspondingly, key_len. 

Stream Format:
*/

 

#include <memory>
#include <boost/asio.hpp>
#include <boost/pointer_cast.hpp>
