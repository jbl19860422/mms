#include <iostream>

#include "server/dtls/dtls_handshake.h"
#include "server/dtls/server_hello.h"
#include "server/dtls/client_hello.h"
#include "server/dtls/extension/dtls_use_srtp.h"

#include "server/dtls/server_certificate.h"
#include "server/dtls/server_hello_done.h"
#include "dtls_ctx.h"
#include "dtls_cert.h"
#include "server/dtls/tls_prf.h"

#include "base/utils/utils.h"
#include "openssl/aes.h"
using namespace mms;

bool DtlsCtx::init()
{
    return true;
}

bool DtlsCtx::processDtlsPacket(uint8_t *data, size_t len, UdpSocket *sock, const boost::asio::ip::udp::endpoint &remote_ep, boost::asio::yield_context & yield)
{
    int32_t consumed = 0;

    // std::vector<DTLSCiphertext> dtls_msgs;
    while (len > 0) {
        DTLSCiphertext dtls_msg;
        consumed = dtls_msg.decode(data, len);
        if (consumed < 0)
        {
            // std::cout << "********************* return false here **********************" << std::endl;
            // std::string finished_data = PRF(master_secret_, "client finished", Utils::sha256(verify_data_), 64);
            // std::cout << "finished_data:" << std::endl;
            // for(int i = 0; i < finished_data.size(); i++) {
            //     printf("%02x", (uint8_t)finished_data[i]);
            // }

            // struct {
            //     opaque IV[SecurityParameters.record_iv_length];
            //     //以下数据加密, 生成消息体
            //     block-ciphered struct {
            //         opaque content[];
            //         opaque MAC[32]; //本例使用HMAC-SHA256, 输出32字节
            //         uint8 padding[GenericBlockCipher.padding_length]; //用于对齐16字节. 填充的内容为padding_length
            //         uint8 padding_length;  //对齐字节的长度
            //         //最终整个个结构体必须是16的倍数.
            //     };
            // } GenericBlockCipher;
            // ————————————————
            // 版权声明：本文为CSDN博主「wzj_whut」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
            // 原文链接：https://blog.csdn.net/wzj_whut/article/details/86626529
            // 去掉头部13字节偏移
            uint16_t data_len = len - 13;
            uint8_t *encrypted_data = data + 13;
            // 获取16字节iv
            std::string iv;
            iv.assign((char*)encrypted_data, 16);
            // 获取加密内容
            unsigned char *content = (unsigned char*)encrypted_data + 16;
            // 执行aes128解密
            AES_KEY key;
            AES_set_decrypt_key((unsigned char*)client_write_key_.data(), 128, &key);
            unsigned char *out = new unsigned char[data_len];
            AES_cbc_encrypt(content, out, data_len - 16, &key, (unsigned char*)iv.data(), AES_DECRYPT);
            for(int i = 0; i < data_len; i++) {
                printf("%02x ", out[i]);
            }
            printf("\r\n");
            return false;
        }

        bool ret = true;
        if (dtls_msg.getType() == handshake)
        {
            verify_data_.append((char*)data, consumed);
            HandShake *handshake = (HandShake *)dtls_msg.msg.get();
            if (handshake->getType() == client_hello)
            {
                ret = processClientHello(dtls_msg, sock, remote_ep, yield);
            }
            else if (handshake->getType() == client_key_exchange)
            {
                ret = processClientKeyExchange(dtls_msg, sock, remote_ep, yield);
            }
        }
        else if (dtls_msg.getType() == change_cipher_spec)
        {
            // verify_data_.append((char*)data, consumed);
            std::cout << "************************** get change_cipher_spec *************************" << std::endl;
        }
        else
        {
            std::string finished_data = PRF(master_secret_, "client finished", Utils::sha256(verify_data_), 64);
            std::cout << "finished_data:" << std::endl;
            for(int i = 0; i < finished_data.size(); i++) {
                printf("%02x", (uint8_t)finished_data[i]);
            }
            printf("\r\n");
            return true;
        }
        
        if (!ret)
        {
            return false;
        }

        data += consumed;
        len -= consumed;
        std::cout << "********************* consumed:" << consumed << ", len:" << len << " **********************" << std::endl;
    }
    std::cout << "len:" << len << std::endl;
    return true;
}

bool DtlsCtx::processClientHello(DTLSCiphertext & recv_msg, UdpSocket *sock, const boost::asio::ip::udp::endpoint &remote_ep, boost::asio::yield_context & yield)
{
    client_hello_ = recv_msg;

    HandShake * recv_handshake_msg = (HandShake *)client_hello_.value().msg.get();
    ClientHello *client_hello = (ClientHello *)recv_handshake_msg->msg.get();
    {// send server hello
        DTLSCiphertext resp_msg;
        resp_msg.setType(handshake);
        resp_msg.setDtlsProtocolVersion(DtlsProtocolVersion(DTLS_MAJOR_VERSION1, DTLS_MINOR_VERSION2));
        resp_msg.setSequenceNo(message_seq_);

        std::unique_ptr<HandShake> resp_handshake = std::unique_ptr<HandShake>(new HandShake);
        auto *s = new ServerHello;
        std::unique_ptr<HandShakeMsg> resp_server_hello = std::unique_ptr<HandShakeMsg>(s);
        std::unique_ptr<UseSRtpExt> use_srtp_ext = std::unique_ptr<UseSRtpExt>(new UseSRtpExt);
        use_srtp_ext->addProfile(SRTP_AES128_CM_HMAC_SHA1_80);
        s->addExtension(std::move(use_srtp_ext));
        s->setDtlsProtocolVersion(DtlsProtocolVersion(DTLS_MAJOR_VERSION1, DTLS_MINOR_VERSION2));
        s->genRandom();
        s->setCipherSuite(TLS_RSA_WITH_AES_128_CBC_SHA);
        resp_handshake->setType(server_hello);
        resp_handshake->setMsg(std::move(resp_server_hello));
        resp_handshake->setMessageSeq(message_seq_);
        resp_msg.setMsg(std::move(resp_handshake));
        auto resp_size = resp_msg.size();

        std::unique_ptr<uint8_t[]> data = std::unique_ptr<uint8_t[]>(new uint8_t[resp_size]);
        int32_t consumed = resp_msg.encode(data.get(), resp_size);
        if (consumed < 0) 
        {// todo:add log
            return false;
        }
        verify_data_.append((char*)data.get(), resp_size);
        sock->sendTo(std::move(data), resp_size, remote_ep, yield);
        message_seq_++;
    }

    {
        DTLSCiphertext resp_msg;
        resp_msg.setType(handshake);
        resp_msg.setDtlsProtocolVersion(DtlsProtocolVersion(DTLS_MAJOR_VERSION1, DTLS_MINOR_VERSION2));
        resp_msg.setSequenceNo(message_seq_);

        std::unique_ptr<HandShake> resp_handshake = std::unique_ptr<HandShake>(new HandShake);
        resp_handshake->setType(certificate);
        auto *s = new ServerCertificate;
        std::unique_ptr<HandShakeMsg> resp_server_certificate = std::unique_ptr<HandShakeMsg>(s);
        s->addCert(DtlsCert::getInstance()->getDer());
        resp_handshake->setMsg(std::move(resp_server_certificate));
        resp_handshake->setMessageSeq(message_seq_);
        resp_msg.setMsg(std::move(resp_handshake));
        auto resp_size = resp_msg.size();

        std::unique_ptr<uint8_t[]> data = std::unique_ptr<uint8_t[]>(new uint8_t[resp_size]);
        int32_t consumed = resp_msg.encode(data.get(), resp_size);
        if (consumed < 0) 
        {// todo:add log
            return false;
        }
        verify_data_.append((char*)data.get(), resp_size);
        sock->sendTo(std::move(data), resp_size, remote_ep, yield);
        message_seq_++;
    }

    {
        DTLSCiphertext resp_msg;
        resp_msg.setType(handshake);
        resp_msg.setDtlsProtocolVersion(DtlsProtocolVersion(DTLS_MAJOR_VERSION1, DTLS_MINOR_VERSION2));
        resp_msg.setSequenceNo(message_seq_);

        std::unique_ptr<HandShake> resp_handshake = std::unique_ptr<HandShake>(new HandShake);
        resp_handshake->setType(server_hello_done);
        auto *s = new ServerHelloDone;
        std::unique_ptr<HandShakeMsg> resp_server_hello_done = std::unique_ptr<HandShakeMsg>(s);
        resp_handshake->setMsg(std::move(resp_server_hello_done));
        resp_handshake->setMessageSeq(message_seq_);
        resp_msg.setMsg(std::move(resp_handshake));
        auto resp_size = resp_msg.size();

        std::unique_ptr<uint8_t[]> data = std::unique_ptr<uint8_t[]>(new uint8_t[resp_size]);
        int32_t consumed = resp_msg.encode(data.get(), resp_size);
        if (consumed < 0) 
        {// todo:add log
            return false;
        }
        verify_data_.append((char*)data.get(), resp_size);
        sock->sendTo(std::move(data), resp_size, remote_ep, yield);
        message_seq_++;
        server_hello_ = resp_msg;
    }

    return true;
}

bool DtlsCtx::processClientKeyExchange(DTLSCiphertext & msg, UdpSocket *sock, const boost::asio::ip::udp::endpoint &remote_ep, boost::asio::yield_context & yield)
{
    HandShake * recv_handshake_msg = (HandShake *)msg.msg.get();
    ClientKeyExchange *client_key_exchange_msg = (ClientKeyExchange *)recv_handshake_msg->msg.get();
    std::string decoded_data;
    decoded_data.resize(1024);
    int n = decryptRSA(client_key_exchange_msg->enc_pre_master_secret.pre_master_secret, decoded_data);
    if (n < 0)
    {
        return false;
    }
    std::string pre_master_secret_raw;
    pre_master_secret_raw.assign(decoded_data.data(), n);
    int consumed = pre_master_secret_.decode((uint8_t*)pre_master_secret_raw.data(), n);
    if (consumed < 0)
    {
        return -1;
    }
    HandShake * recv_client_hello_msg = (HandShake *)client_hello_.value().msg.get();
    ClientHello *client_hello = (ClientHello *)recv_client_hello_msg->msg.get();
    if (client_hello->client_version != pre_master_secret_.client_version)
    {
        return false;
    }
    // 生成master secret
    HandShake * send_server_hello_msg = (HandShake *)server_hello_.value().msg.get();
    ServerHello *server_hello = (ServerHello *)send_server_hello_msg->msg.get();
    std::string seed;
    seed.append((char*)client_hello->random.random_raw, 32);
    seed.append((char*)server_hello->random.random_raw, 32);
    master_secret_ = PRF(pre_master_secret_raw, "master secret", seed, 48);
    memcpy(security_params_.master_secret, master_secret_.data(), 48);
    memcpy(security_params_.client_random, client_hello->random.random_raw, 32);
    memcpy(security_params_.server_random, server_hello->random.random_raw, 32);
    // 生成 srtp key material
    // @doc rfc5764 4.1.2.  SRTP Protection Profiles
    //     SRTP_AES128_CM_HMAC_SHA1_80
    //          cipher: AES_128_CM
    //          cipher_key_length: 128
    //          cipher_salt_length: 112
    //          maximum_lifetime: 2^31
    //          auth_function: HMAC-SHA1
    //          auth_key_length: 160
    //          auth_tag_length: 80
    //    SRTP_AES128_CM_HMAC_SHA1_32
    //          cipher: AES_128_CM
    //          cipher_key_length: 128
    //          cipher_salt_length: 112
    //          maximum_lifetime: 2^31
    //          auth_function: HMAC-SHA1
    //          auth_key_length: 160
    //          auth_tag_length: 32
    //          RTCP auth_tag_length: 80
    //    SRTP_NULL_HMAC_SHA1_80
    //          cipher: NULL
    //          cipher_key_length: 0
    //          cipher_salt_length: 0
    //          maximum_lifetime: 2^31
    //          auth_function: HMAC-SHA1
    //          auth_key_length: 160
    //          auth_tag_length: 80
    // 生成srtp key
    size_t srtp_cipher_key_length = 16;//128/8
    size_t srtp_cipher_salt_length = 14;//112/8
    // total:(16+14)*2
    size_t total_key_material_need = (srtp_cipher_key_length + srtp_cipher_salt_length)*2;
    std::string srtp_key_block = PRF(master_secret_, "EXTRACTOR-dtls_srtp", seed, total_key_material_need);

    size_t offset = 0;
    std::string client_master_key((char*)(srtp_key_block.data()), srtp_cipher_key_length);
    offset += srtp_cipher_key_length;
    std::string server_master_key((char*)(srtp_key_block.data() + offset), srtp_cipher_key_length);
    offset += srtp_cipher_key_length;
    std::string client_master_salt((char*)(srtp_key_block.data() + offset), srtp_cipher_salt_length);
    offset += srtp_cipher_salt_length;
    std::string server_master_salt((char*)(srtp_key_block.data() + offset), srtp_cipher_salt_length);

    recv_key_ = client_master_key + client_master_salt;
    send_key_ = server_master_key + server_master_salt;
    // @https://datatracker.ietf.org/doc/html/rfc5246#page-95
    // 生成key block及key material
    //    To generate the key material, compute

    //   key_block = PRF(SecurityParameters.master_secret,
    //                   "key expansion",
    //                   SecurityParameters.server_random +
    //                   SecurityParameters.client_random);
    //                       Key      IV   Block
    // Cipher        Type    Material  Size  Size
    // ------------  ------  --------  ----  -----
    // NULL          Stream      0       0    N/A
    // RC4_128       Stream     16       0    N/A
    // 3DES_EDE_CBC  Block      24       8      8
    // AES_128_CBC   Block      16      16     16
    // AES_256_CBC   Block      32      16     16

    // MAC       Algorithm    mac_length  mac_key_length
    // --------  -----------  ----------  --------------
    // NULL      N/A              0             0
    // MD5       HMAC-MD5        16            16
    // SHA       HMAC-SHA1       20            20
    // SHA256    HMAC-SHA256     32            32
    const int32_t key_size = 16;
    const int32_t iv_size = 16;
    const int32_t mac_size = 20; 
    std::string seed1;
    seed1.append((char*)server_hello->random.random_raw, 32);
    seed1.append((char*)client_hello->random.random_raw, 32);
    int32_t key_block_size = 2*(key_size + iv_size + mac_size);// AES_128_CBC AND SHA
    std::string key_block = PRF(master_secret_, "key expansion", seed1, key_block_size);
    int32_t off = 0;
    client_write_MAC_key_.assign(key_block.data() + off, mac_size);
    off += mac_size;
    server_write_MAC_key_.assign(key_block.data() + off, mac_size);
    off += mac_size;
    client_write_key_.assign(key_block.data() + off, key_size);
    off += key_size;
    server_write_key_.assign(key_block.data() + off, key_size);
    off += key_size;
    client_write_IV_.assign(key_block.data() + off, iv_size);
    off += iv_size;
    server_write_IV_.assign(key_block.data() + off, iv_size);


    uint8_t clientRandom[32] = {
        0x36,0xc1,0x29,0xd0,0x1a,0x32,0x00,0x89,0x4b,0x91,0x79,0xfa,0xac,0x58,0x9d,0x98,0x35,0xd5,0x87,0x75,
        0xf9,0xb5,0xea,0x35,0x87,0xcb,0x8f,0xd0,0x36,0x4c,0xae,0x8c
    };

    uint8_t serverRandom[32] = {
        0xf6,0xc9,0x57,0x5e,0xd7,0xdd,0xd7,0x3e,0x1f,0x7d,0x16,0xec,0xa1,0x15,0x41,0x58,0x12,0xa4,0x3c,
        0x2b,0x74,0x7d,0xaa,0xaa,0xe0,0x43,0xab,0xfb,0x50,0x05,0x3f,0xce
    };
    // uint8_t clientRandom[32] = {
    //      0x5c, 0x48, 0x6a, 0x33, 0x93, 0x14, 0xeb, 0x56, 0xd2, 0x7d, 0x86, 0x07, 0x56, 0x83, 0x80, 0x95,
    //      0xff, 0x1e, 0x3c, 0x32, 0xb4, 0x21, 0xd0, 0x70, 0xab, 0xe2, 0xfb, 0xa0, 0x6a, 0xb2, 0x6a, 0x3f};

    // uint8_t serverRandom[32] = {
    //      0x5c, 0x48, 0x6a, 0x36, 0xa6, 0x57, 0x51, 0xc6, 0x13, 0xa2, 0x88, 0xdf, 0x39, 0xa0, 0x42, 0x02,
    //      0x9c, 0x7b, 0x35, 0xcb, 0x4f, 0x55, 0x5d, 0x91, 0x3c, 0x7f, 0x89, 0xe2, 0xc6, 0x21, 0xe2, 0xeb};

    // uint8_t preMaster[48] = {
    //     0x03,0x03,0x5f,0x60,0x30,0x50,0x2c,0xe2,0x75,0xdf,0x0a,0x4c,0x13,0x65,0x62,0xe5,
    //     0x5d,0xdb,0x0f,0x08,0x36,0x5e,0xa9,0xe7,0xc2,0xb4,0x25,0x50,0x28,0x4a,0x4b,0xd9,
    //     0x66,0x4b,0x4c,0xc8,0x93,0xfe,0xbc,0x48,0x44,0xcf,0xd6,0xe2,0x31,0xde,0x53,0x7b
    // };
    // uint8_t preMaster[48] = {
    //     0x86, 0x05, 0x19, 0x48, 0xe4, 0xd9, 0xa0, 0xcd, 0x27, 0x3b, 0x6c, 0xd3, 0xa7, 0x65,
    //     0x57, 0xfc, 0x69, 0x5e, 0x2a, 0xd9, 0x51, 0x7c, 0xda, 0x97, 0x08, 0x1e, 0xd0, 0x09,
    //     0x58, 0x8a, 0x20, 0xab, 0x48, 0xd0, 0xb1, 0x28, 0xde, 0x8f, 0x91, 0x7d, 0xa7, 0x4e, 
    //     0x71, 0x18, 0x79, 0x46, 0x0b, 0x60
    // };
    uint8_t preMaster[48] = {
        0xf8,0x93,0x8e,0xcc,0x9e,0xde,0xbc,0x50,0x30,0xc0,0xc6,0xa4,0x41,0xe2,0x13,0xcd,0x24,0xe6,0xf7,
        0x70,0xa5,0x0d,0xda,0x07,0x87,0x6f,0x8d,0x55,0xda,0x06,0x2b,0xca,0xdb,0x38,0x6b,0x41,0x1f,0xd4,
        0xfe,0x43,0x13,0xa6,0x04,0xfc,0xe6,0xc1,0x7f,0xbc
    };
    std::string seed3;
    seed3.append((char*)clientRandom, 32);
    seed3.append((char*)serverRandom, 32);
    std::string master_secret = PRF(std::string((char*)preMaster, 48), "master secret", seed3, 48);
    printf("master secret:\r\n");
    for(int i = 0; i < 48; i++) {
        printf("%02x ", (uint8_t)master_secret[i]);
    }
    printf("\r\n");
    return true;
}

int32_t DtlsCtx::decryptRSA(const std::string & enc_data, std::string & dec_data)
{
    int num = RSA_private_decrypt(RSA_size(DtlsCert::getInstance()->getRSA()), (const uint8_t*)enc_data.data(), (uint8_t*)dec_data.data(), DtlsCert::getInstance()->getRSA(), RSA_PKCS1_PADDING);
    return num;
}

bool DtlsCtx::calcMasterSecret()
{
    
}