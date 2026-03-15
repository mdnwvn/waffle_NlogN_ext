#ifndef NLN_CLIENT_H
#define NLN_CLIENT_H

#include "waffle_thrift.h"

#include "waffle/command_response_reader.h"


#define GET 0
#define PUT 1
#define GET_BATCH 2
#define PUT_BATCH 3

#include <thread>
#include "waffle/queue.h"

class nln_client
{
public:
    nln_client(std::string host, int port);

    int64_t get_client_id();
    // std::string get_level(const std::string &key);

    void put_batch(const std::vector<std::string> &keys, const std::vector<std::string> &values);
    void get_batch(const std::vector<std::string> &keys);

protected:
    int batch_size = 2000;
    std::string host;
    int port;

    int64_t sequence_num = 0;
    int64_t client_id_;
    sequence_id seq_id_;

    std::condition_variable *m_cond_;
    std::mutex *m_mtx_;

    std::shared_ptr<WaffleQueue::queue<int>> requests_;
    std::atomic_int *total_;
    std::atomic_bool *done_;

    std::shared_ptr<waffle_thriftConcurrentClient> client_;
    std::thread *response_thread_;

    int in_flight_limit_ = 2000;

    command_response_reader reader_;

    /* Transport */
    std::shared_ptr<apache::thrift::transport::TTransport> transport_{};
    /* Protocol */
    std::shared_ptr<apache::thrift::protocol::TProtocol> protocol_{};

    void read_responses();
};

class lookup_client : public nln_client
{

    void get_batch(const std::vector<std::string> &keys);
};

class level_client : public nln_client
{

    void get_batch(const std::vector<std::string> &keys);
};

#endif