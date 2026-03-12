#ifndef NLN_CLIENT_H
#define NLN_CLIENT_H

#include "waffle_thrift.h"

class nln_client
{
public:
    nln_client(std::string host, int port);

    std::string get_level(const std::string &key);

    void put_batch(const std::vector<std::string> &keys, const std::vector<std::string> &values);
    std::vector<std::string> get_batch(const std::vector<std::string> &keys);

private:
    std::string host;
    int port;

    std::shared_ptr<waffle_thriftConcurrentClient> client_;
};

#endif