#ifndef NLN_ACTOR_H
#define NLN_ACTOR_H
// Manages forwarding requests to the backend Waffle servers and sending responses back as well.

#include "nln_client.h"

#include <atomic>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <thread>
#include <future>
#include <random>
#include <unordered_set>
#include <string>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>
#include <iostream>
#include <chrono>
#include <math.h>
#include "unistd.h"

#include "waffle/thrift_response_client_map.h"
#include "waffle/queue.h"
#include "waffle/operation.h"

class nln_proxy
{

public:
    void init(void **args);

    std::string get(const std::string &key);
    void put(const std::string &key, const std::string &value);
    std::vector<std::string> get_batch(const std::vector<std::string> &keys);
    void put_batch(const std::vector<std::string> &keys, const std::vector<std::string> &values);

    std::string get(int queue_id, const std::string &key);
    void put(int queue_id, const std::string &key, const std::string &value);
    std::vector<std::string> get_batch(int queue_id, const std::vector<std::string> &keys);
    void put_batch(int queue_id, const std::vector<std::string> &keys, const std::vector<std::string> &values);

    void async_get(const sequence_id &seq_id, const std::string &key);
    void async_put(const sequence_id &seq_id, const std::string &key, const std::string &value);
    void async_get_batch(const sequence_id &seq_id, const std::vector<std::string> &keys);
    void async_put_batch(const sequence_id &seq_id, const std::vector<std::string> &keys, const std::vector<std::string> &values);

    void async_get(const sequence_id &seq_id, int queue_id, const std::string &key);
    void async_put(const sequence_id &seq_id, int queue_id, const std::string &key, const std::string &value);
    void async_get_batch(const sequence_id &seq_id, int queue_id, const std::vector<std::string> &keys);
    void async_put_batch(const sequence_id &seq_id, int queue_id, const std::vector<std::string> &keys, const std::vector<std::string> &values);

    std::future<std::string> get_future(int queue_id, const std::string &key);
    std::future<std::string> put_future(int queue_id, const std::string &key, const std::string &value);

    int num_cores = 1;

private:
    void create_security_batch(std::shared_ptr<WaffleQueue::queue<std::pair<operation, std::shared_ptr<std::promise<std::string>>>>> &op_queue,
                               std::vector<operation> &storage_batch,
                               std::unordered_map<std::string, std::vector<std::shared_ptr<std::promise<std::string>>>> &keyToPromiseMap, int &cacheMisses);

    void consumer_thread(int id);
    void responder_thread();
    std::vector<std::thread> threads_;
    std::shared_ptr<thrift_response_client_map> id_to_client_;
    std::vector<std::shared_ptr<WaffleQueue::queue<std::pair<operation, std::shared_ptr<std::promise<std::string>>>>>> operation_queues_;

    std::shared_ptr<nln_client> level_map_client_ ;
    std::vector<std::shared_ptr<nln_client>> levels_clients_;

    int GET = 0;
    int PUT = 1;
    int GET_BATCH = 2;

    bool finished_ = false;
    int PUT_BATCH = 3;
    

    WaffleQueue::queue<std::pair<int, std::pair<const sequence_id &, std::vector<std::future<std::string>>>>> respond_queue_;
    WaffleQueue::queue<sequence_id> sequence_queue_;
};

#endif