#pragma once

#define BUFF_SIZE          1024

#define READ_BUFF_SIZE  BUFF_SIZE * 10

#define MIN_VERSION           1

#define STAGE_INIT       0x0001
#define STAGE_CRYPT      0x0002

#define STAGE_UPLOAD     0x0004
#define STAGE_DOWNLOAD   0x0008
#define STAGE_WAITING    0x0010

#define STAGE_DESTORYED  0x0020

#define CRYPT_HEADER_SIZE       4
#define HELLO_HEADER_SIZE       2
/*
one batch by one batch send and store
+-----------------------------------------------+
|       read one batch data                     |
|       stage_init(send auth, build connection) |<-------------------------------+
|       change stage to encrypt                 |                                |
+-----------------------------------------------+                                |
                    |                                                            |
                    V                                                            |
+-----------------------------------------------+       +-------------------------------------------------------+
|       stage_crypt(get sym key)                |       | stage_wait(wait until encrypt data finish)            |
|       encrypt data batch by batch)            | —————>| handler sleep until be wakened                        |
|       change stage to upload                  |       | change stage to init, upload according to connection  |
+-----------------------------------------------+       +-------------------------------------------------------+
                                                                                |
                                                                                |
                                                                                |
+-----------------------------------------------+                               |
|       stage_upload(upload one batch)          |<------------------------------+
|       change stage to download                |
+-----------------------------------------------+   
                    |
                    V
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



* ALL PACKAGE IS ENCODE BY BASE64 DUE TO ADD BORDER BUFFER '\n' *

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
+-------+-------+--------+---------+----------+
|  VER  |  CMD  |  MODE  |  N_LEN  |   DATA   |
+-------+-------+--------+---------+----------+
|   1   |   1   |   1    |    1    | Variable |
+-------+-------+--------+---------+----------+

VER:        protocol version: 0x01
CMD:
    0x01:   client: request for new key. 
    0x02:   client: check the key id uploaded in data if is available.
    
    0x11:   server:  send the new key.
    0x12:   server:  return the key id works.

MODE: first four bits represent cipher, second four bits represent asymmetric cryptography

    0x01: AES_128_GCM 

    0x10: RSA_OAEP


size field denotes: x for x * 1024, e.g. 2 for 2 * 1024 = 2048
N_LEN:      n len of public key, 0 for others, correspondingly, e_len is data_len - n_len, or
            n len for symmtric cipher key length, correspondingly, iv_len is data_len - n_len.


Stream Format:
*/

 

#include <memory>
#include <boost/asio.hpp>
#include <boost/pointer_cast.hpp>
