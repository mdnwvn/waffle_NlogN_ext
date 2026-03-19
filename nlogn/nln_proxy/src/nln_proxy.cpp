#include "nln_proxy.h"
#include "waffle/operation.h"
#include "nln_client.h"
#include "nln_levels.h"

#include <fstream>
#include <regex>
#include <assert.h>

void nln_proxy::init(void **args)
{

    id_to_client_ = *(static_cast<std::shared_ptr<thrift_response_client_map> *>(args[0]));
    // level_map_client_ = *(static_cast<std::shared_ptr<lookup_client> *>(args[1]));
    levels_clients_ = *(static_cast<std::vector<std::shared_ptr<level_client>> *>(args[2]));

    lookup_table = std::make_shared<std::unordered_map<std::string, int>>();
    std::ifstream lookup_file(lookup_table_path);
    assert(lookup_file.is_open());

    std::regex rgx("^(SET)\\s(\\S*)\\s((\\d+).*)$");
    std::smatch line_matches;

    std::string line;
    while (std::getline(lookup_file, line))
    {
        if (std::regex_match(line, line_matches, rgx))
        {
            assert(line_matches.size() == 5);
            assert(line_matches[1] == "SET");

            char *end;
            int index = strtol(line_matches[4].str().c_str(), &end, 10);
            assert(end != nullptr);

            lookup_table->insert(std::make_pair(line_matches[2], index));
        }
        else
        {
            throw std::runtime_error("Bad regex match");
        }
    }
    lookup_file.close();

    // int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    std::cout << "max cores is " << sysconf(_SC_NPROCESSORS_ONLN) << std::endl
              << " and current cores used is " << num_cores;
    std::vector<std::thread> threads;

    std::vector<std::string> keysCacheUnencrypted;
    std::vector<std::string> valuesCache;

    cache = Cache(keysCacheUnencrypted, valuesCache, (100 * 100000) * 10);

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

    std::unordered_map<int, std::pair<std::vector<std::string>, std::vector<std::string>>> level_split;

    for (int i = 0; i < keys.size(); i++)
    {
        int index = lookup_table->find(keys[i])->second;
        auto slot = level_split.find(index);
        // std::cout << strtol(_return[i].c_str(), &end, 10) << std::endl;
        if (slot != level_split.end())
        {

            slot->second.first.push_back(keys[i]);
        }
        else
        {
            level_split.insert(std::make_pair(index, std::make_pair(std::vector<std::string>(), std::vector<std::string>())));
            slot = level_split.find(index);
            slot->second.first.push_back(keys[i]);
        }
    }

    for (auto &it : level_split)
    {
        // Do stuff
        // cout << it.first;

        if (it.first < levels_len)
        {
            if (levels[it.first].exists)
            {
                levels_clients_[it.first]->get_batch(it.second.first);
            }
        }
    }

    // level_map_client_->get_batch(keys);
};

void nln_proxy::async_put_batch(const sequence_id &seq_id, int queue_id, const std::vector<std::string> &keys, const std::vector<std::string> &values)
{
    // Send waiters to responder thread
    std::vector<std::future<std::string>> waiters;
    // std::cout << "async_put_batch client ID is " << seq_id.client_id << std::endl;
    int i = 0;
    for (const auto &key : keys)
    {
        waiters.push_back((put_future(queue_id, key, values[i])));
        i++;
    }

    respond_queue_.push(std::make_pair(PUT_BATCH, std::make_pair(seq_id, std::move(waiters))));
    sequence_queue_.push(seq_id);

    std::unordered_map<int, std::pair<std::vector<std::string>, std::vector<std::string>>> level_split;

    for (int i = 0; i < keys.size(); i++)
    {
        int index = lookup_table->find(keys[i])->second;
        auto slot = level_split.find(index);
        // std::cout << strtol(_return[i].c_str(), &end, 10) << std::endl;
        if (slot != level_split.end())
        {

            slot->second.first.push_back(keys[i]);
        }
        else
        {
            level_split.insert(std::make_pair(index, std::make_pair(std::vector<std::string>(), std::vector<std::string>())));
            slot = level_split.find(index);
            slot->second.first.push_back(keys[i]);
            slot->second.second.push_back(values[i]);
        }
    }

    for (auto &it : level_split)
    {

        if (it.first < levels_len)
        {
            if (levels[it.first].exists)
            {
                levels_clients_[it.first]->put_batch(it.second.first, it.second.second);
            }
        }
    }
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

std::future<std::string> nln_proxy::put_future(int queue_id, const std::string &key, const std::string &value)
{
    auto prom = std::make_shared<std::promise<std::string>>();
    std::future<std::string> waiter = prom->get_future();
    struct operation operat;
    operat.key = key;
    operat.value = value;
    operation_queues_[queue_id % operation_queues_.size()]->push(std::make_pair(operat, prom));
    return waiter;
};

void nln_proxy::insert_into_cache(const std::vector<std::string> &keys, const std::vector<std::string> &values)
{

    for (int i = 0; i < keys.size(); i++)
    {
        cache.insertIntoCache(keys[i], values[i]);
    }
}

void nln_proxy::resolve_promise(std::shared_ptr<WaffleQueue::queue<std::pair<operation, std::shared_ptr<std::promise<std::string>>>>> &op_queue,
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

        if (operation_promise_pair.first.value == "")
        {

            bool isPresentInCache = false;
            auto val = cache.getValueWithoutPositionChangeNew(currentKey, isPresentInCache);
            if (isPresentInCache == true)
            {
                operation_promise_pair.second->set_value(val);
            }
            else
            {
                // Push the operation back onto the queue and record a cache miss.
                op_queue->push(operation_promise_pair);
                cacheMisses += 1;
            }
        }
        else
        {
            cache.insertIntoCache(currentKey, operation_promise_pair.first.value);
            operation_promise_pair.second->set_value(operation_promise_pair.first.value);
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

                resolve_promise(operation_queues_[id], storage_batch, keyToPromiseMap, cacheMisses);
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