#include "nln_proxy.h"
#include "waffle/operation.h"
#include "nln_client.h"

void nln_proxy::init(void **args)
{

    id_to_client_ = *(static_cast<std::shared_ptr<thrift_response_client_map> *>(args[0]));
    level_map_client_ = *(static_cast<std::shared_ptr<nln_client> *>(args[1]));
    levels_clients_ = *(static_cast<std::vector<std::shared_ptr<nln_client>> *>(args[2]));

    // int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    std::cout << "max cores is " << sysconf(_SC_NPROCESSORS_ONLN) << std::endl
              << " and current cores used is " << num_cores;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_cores; i++)
    {
        auto q = std::make_shared<WaffleQueue::queue<std::pair<operation, std::shared_ptr<std::promise<std::string>>>>>();
        operation_queues_.push_back(q);
    }
    for (int i = 0; i < num_cores; i++)
    {
        threads_.push_back(std::thread(&nln_proxy::consumer_thread, this, i));
    }

    threads_.push_back(std::thread(&nln_proxy::responder_thread, this));
}

void nln_proxy::async_get_batch(const sequence_id &seq_id, int queue_id, const std::vector<std::string> &keys)
{
    std::vector<std::string> _return;
    std::vector<std::future<std::string>> waiters;
    for (const auto &key : keys)
    {
        waiters.push_back((get_future(queue_id, key)));
    }
    // std::cout << "async_get_batch client ID is " << seq_id.client_id << std::endl;
    respond_queue_.push(std::make_pair(GET_BATCH, std::make_pair(seq_id, std::move(waiters))));
    sequence_queue_.push(seq_id);
};

std::future<std::string> nln_proxy::get_future(int queue_id, const std::string &key)
{
    auto prom = std::make_shared<std::promise<std::string>>();
    std::future<std::string> waiter = prom->get_future();
    struct operation operat;
    operat.key = key;
    operat.value = "";
    operation_queues_[queue_id % operation_queues_.size()]->push(std::make_pair(operat, prom));
    return waiter;
};

void nln_proxy::create_security_batch(std::shared_ptr<WaffleQueue::queue<std::pair<operation, std::shared_ptr<std::promise<std::string>>>>> &op_queue,
                                      std::vector<operation> &storage_batch,
                                      std::unordered_map<std::string, std::vector<std::shared_ptr<std::promise<std::string>>>> &keyToPromiseMap, int &cacheMisses)
{

    if (op_queue->size() == 0)
    {
        std::cout << "WARNING: You should never see this line on console! Queue size is 0" << std::endl;
    }
    else
    {
        struct operation operat;
        auto operation_promise_pair = op_queue->pop();
        auto currentKey = operation_promise_pair.first.key;
        // std::cout << operation_promise_pair.first.value << std::endl;
        if (operation_promise_pair.first.value == "")
        {
            // printf("hi\n");

            // TODO: actually call a backend server to get the values.
            operation_promise_pair.second->set_value("test");

            // It's a GET request
            // bool isPresentInCache = false;
            // auto val = cache.getValueWithoutPositionChangeNew(currentKey, isPresentInCache);
            // auto valEvicted = EvictedItems.getValue(currentKey);
            // if(isPresentInCache == true) {
            //    operation_promise_pair.second->set_value(val);
            //}
            // else if(valEvicted != "") {
            //    operation_promise_pair.second->set_value(valEvicted);
            //} else {
            //    auto isPresentInRunningKeys = runningKeys.insertIfNotPresent(currentKey, operation_promise_pair.second);
            //    if(isPresentInRunningKeys == false) {
            //        storage_batch.push_back(operation_promise_pair.first);
            //    }
            //    ++cacheMisses;
            //}
        }
        else
        {
            // TODO: actually implement putting keys to the backend.

            //// It's a PUT request
            // if(cache.checkIfKeyExists(currentKey) == false && EvictedItems.checkIfKeyExists(currentKey) == false) {
            //     auto isPresentInRunningKeys = runningKeys.insertIfNotPresent(currentKey);
            //     if(isPresentInRunningKeys == false) {
            //         storage_batch.push_back(operation_promise_pair.first);
            //     }
            //     ++cacheMisses;
            // }
            // cache.insertIntoCache(currentKey, operation_promise_pair.first.value);
            // operation_promise_pair.second->set_value(cache.getValueWithoutPositionChange(currentKey));
        }
    }
};

void nln_proxy::consumer_thread(int id)
{
    int operations_serviced = 0;
    int previous_total_operations = 0;
    int total_operations = 0;
    int batchSize = 20;
    std::cout << "Entering here to consumer thread" << std::endl;
    while (!finished_)
    {
        std::vector<operation> storage_batch;
        std::unordered_map<std::string, std::vector<std::shared_ptr<std::promise<std::string>>>> keyToPromiseMap;
        // std::unordered_set<std::string> tempSet;
        int i = 0;
        int cacheMisses = 0;

        while (i < batchSize && !finished_)
        {
            if (operation_queues_[id]->size() > 0)
            {

                create_security_batch(operation_queues_[id], storage_batch, keyToPromiseMap, cacheMisses);
                ++i;
            }
        }
    }
};

void nln_proxy::responder_thread()
{
    while (true)
    {
        auto tuple = respond_queue_.pop();
        auto op_code = tuple.first;
        auto seq = tuple.second.first;
        seq = sequence_queue_.pop();
        std::vector<std::string> results;
        for (int i = 0; i < tuple.second.second.size(); i++)
        {
            // std::cout << "Responding to " << seq << "with " << tuple.second.second[i].get() <<std::endl;
            results.push_back(tuple.second.second[i].get());
        }
        id_to_client_->async_respond_client(seq, op_code, results);
    }
    std::cout << "Quitting response thread" << std::endl;
};