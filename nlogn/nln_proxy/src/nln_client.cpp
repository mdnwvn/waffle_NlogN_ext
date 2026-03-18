

#include "nln_client.h"

#include "waffle/queue.h"
#include "nln_levels.h"
#include "nln_proxy.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <iostream>
#include <regex>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

nln_client::nln_client(std::string host, int port) : host(host), port(port)
{
  done_ = new std::atomic<bool>(false);
  total_ = new std::atomic<int>(0);

  auto socket = std::make_shared<TSocket>(host, port);
  socket->setRecvTimeout(10000);
  socket->setSendTimeout(1200000);
  transport_ = std::shared_ptr<TTransport>(new TFramedTransport(socket));
  protocol_ = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
  client_ = std::make_shared<waffle_thriftConcurrentClient>(protocol_);
  transport_->open();
  requests_ = std::make_shared<WaffleQueue::queue<int>>();
  client_id_ = get_client_id();
  seq_id_ = sequence_id();
  // std::cout << "IN client::init, client_id_ is " << client_id_ << std::endl;
  seq_id_.__set_client_id(client_id_);

  m_mtx_ = new std::mutex();
  m_cond_ = new std::condition_variable();

  reader_ = command_response_reader(protocol_);

  // std::cout << "in constructor" << std::endl;
  // std::shared_ptr<TTransport> socket(new TSocket(host, port));

  // std::cout << "Socket created " << std::endl;
  // std::shared_ptr<TTransport> transport(new TFramedTransport(socket));
  // std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  // std::cout << "Storing pointer" << std::endl;

  // std::shared_ptr<waffle_thriftConcurrentClient> client_test_ = ;

  // transport->open();

  // std::string res;
  // std::string key = "user451800245123463256";
  // std:: cout << "getting key" << std::endl;
  // client_->get(res, key);

  std::cout << "Client created " << std::endl;
}

lookup_client::lookup_client(std::string host, int port, void **args) : nln_client(host, port)
{

  levels_clients_ = *(static_cast<std::vector<std::shared_ptr<level_client>> *>(args[0]));

  response_thread_ = new std::thread(&lookup_client::read_responses, this);
}

level_client::level_client(std::string host, int port, void **args) : nln_client(host, port)
{

  proxy_ = *(static_cast<std::shared_ptr<nln_proxy> *>(args[0]));

  response_thread_ = new std::thread(&level_client::read_responses, this);
}

// std::string nln_client::get_level(const std::string &key)
//{
//   std::string res;
//   client_->get(res, key);
//   return res;
// }

int64_t nln_client::get_client_id()
{
  auto id = client_->get_client_id();
  auto block_id_ = 1;
  // std::cout << "Client ID is " << id << std::endl;
  client_->register_client_id(block_id_, id);
  return id;
}

// void lookup_client::get_batch(const std::vector<std::string> &keys)
// {
//   std::vector<std::string> _res;
//   seq_id_.__set_client_seq_no(sequence_num++);
//   client_->get_batch(_res, keys);
// }
//
// void level_client::get_batch(const std::vector<std::string> &keys)
// {
//
//   std::vector<std::string> _res;
//   seq_id_.__set_client_seq_no(sequence_num++);
//   client_->get_batch(_res, keys);
// }

void lookup_client::get_batch(const std::vector<std::string> &keys)
{
  std::unique_lock<std::mutex> mlock(*m_mtx_);
  // std::cout << "Entering async_proxy_client.cpp line " << __LINE__ << "requests size is " << requests_->size() << std::endl;
  while (requests_->size() >= in_flight_limit_)
  {
    // std::cout << "Waiting for lock " <<std::endl;
    m_cond_->wait(mlock);
  }

  // std::cout << "client get_batch" << std::endl;
  std::vector<std::string> _return;
  seq_id_.__set_client_seq_no(sequence_num++);
  client_->async_get_batch(seq_id_, keys);
  requests_->push(GET_BATCH);

  pending_get_requests.insert(std::make_pair(seq_id_.client_seq_no, keys));
}

void level_client::get_batch(const std::vector<std::string> &keys)
{
  std::unique_lock<std::mutex> mlock(*m_mtx_);
  while (requests_->size() >= in_flight_limit_)
  {
    // std::cout << "Waiting for lock " <<std::endl;
    m_cond_->wait(mlock);
  }
  // std::unique_lock<std::mutex> mlock(*m_mtx_);
  //  std::cout << "Entering async_proxy_client.cpp line " << __LINE__ << "requests size is " << requests_->size() << std::endl;

  // std::cout << "client get_batch" << std::endl;
  std::vector<std::string> _return;
  seq_id_.__set_client_seq_no(sequence_num++);
  client_->async_get_batch(seq_id_, keys);
  requests_->push(GET_BATCH);

  pending_get_requests.insert(std::make_pair(seq_id_.client_seq_no, keys));
}

void lookup_client::put_batch(const std::vector<std::string> &keys, const std::vector<std::string> &values)
{
  std::unique_lock<std::mutex> mlock(*m_mtx_);
  while (requests_->size() >= in_flight_limit_)
  {
    // std::cout << "Waiting for lock " <<std::endl;
    m_cond_->wait(mlock);
  }
  std::string _return;
  seq_id_.__set_client_seq_no(sequence_num++);
  client_->async_put_batch(seq_id_, keys, values);
  requests_->push(PUT_BATCH);

  pending_put_requests.insert(std::make_pair(seq_id_.client_seq_no, std::make_pair(keys, values)));
}

// void nln_client::get_batch(const std::vector<std::string> &keys)
//{
//   // std::cout << "client get_batch" << std::endl;
//   std::vector<std::string> _return;
//   seq_id_.__set_client_seq_no(sequence_num++);
//   client_->async_get_batch(seq_id_, keys);
//   requests_->push(GET_BATCH);
//
//   pending_get_requests.insert(std::make_pair(seq_id_.client_seq_no, keys));
// }

void nln_client::put_batch(const std::vector<std::string> &keys, const std::vector<std::string> &values)
{
  std::string _return;
  seq_id_.__set_client_seq_no(sequence_num++);
  client_->async_put_batch(seq_id_, keys, values);
  requests_->push(PUT_BATCH);

  pending_put_requests.insert(std::make_pair(seq_id_.client_seq_no, std::make_pair(keys, values)));
}

void lookup_client::read_responses()
{
  std::cout << "Client read responses is called " << std::endl;
  std::vector<std::string> _return;
  while (!done_->load())
  {
    auto type = requests_->pop();
    m_cond_->notify_one();
    try
    {

      // std::cout << "recv'd response?" << std::endl;

      int64_t id = reader_.recv_response(_return);
      auto found = pending_get_requests.find(id);
      if (found != pending_get_requests.end())
      {
        std::unordered_map<int, std::pair<std::shared_ptr<std::vector<std::string>>, std::shared_ptr<std::vector<std::string>>>> responses;
        for (int i = 0; i < _return.size(); i++)
        {

          std::regex padded_number("^(\\d+)(█+.*)?$");
          std::smatch matches;

          if (std::regex_search(_return[i], matches, padded_number))
          {
            // std::cout << "Match found\n";

            // for (size_t i = 0; i < matches.size(); ++i) {
            //     std::cout << i << ": '" <<  << "'\n";
            // }

            char *end;
            int index = strtol(matches[1].str().c_str(), &end, 10);

            if (end != nullptr)
            {
              auto slot = responses.find(index);
              // std::cout << strtol(_return[i].c_str(), &end, 10) << std::endl;
              if (slot != responses.end())
              {

                slot->second.first->push_back(found->second[i]);
              }
              else
              {
                responses.insert(std::make_pair(index, std::make_pair(std::make_shared<std::vector<std::string>>(), std::make_shared<std::vector<std::string>>())));
                slot = responses.find(index);
                slot->second.first->push_back(found->second[i]);
              }
            }
          }
          else
          {
            // std::cout << "Match not found | " << _return[i] << " \n";
          }
        }

        // NOTE: removing this debug output will break things...
        std::cout << "LO -> " << _return[0] << " | " << id << " | " << type << " | " << found->second[0] << " | " << _return.size() << " " << found->second.size() << std::endl;

        // std::cout << responses.size() << std::endl;

        // this doesnt quite work yet
        for (auto &it : responses)
        {
          // Do stuff
          // cout << it.first;
          std::vector<std::string> thing(*(it.second.first));

          if (it.first < levels_len)
          {
            if (levels[it.first].exists)
            {

              levels_clients_[it.first]->get_batch(thing);
            }
          }
        }
      }
      else

        std::cout << "LO -> " << _return[0] << " | " << id << " | " << type << " | Not pending?" << std::endl;
    }
    catch (apache::thrift::transport::TTransportException e)
    {
      // std::cout << "Client read responses is FAILURE " << std::endl;
      (void)0;
    }
    *total_ += _return.size();
    _return.clear();
  }
}

void level_client::read_responses()
{
  std::cout << "Client read responses is called " << std::endl;
  std::vector<std::string> _return;
  while (!done_->load())
  {
    auto type = requests_->pop();
    m_cond_->notify_one();
    try
    {

      // std::cout << "recv'd response?" << std::endl;

      int64_t id = reader_.recv_response(_return);
      auto found = pending_get_requests.find(id);
      if (found != pending_get_requests.end())
      {
        std::cout << "LE -> " << _return[0] << " | " << id << " | " << type << " | " << found->second[0] << " | " << _return.size() << " " << found->second.size() << std::endl;

        proxy_->insert_into_cache(found->second, _return);
      }
      else

        std::cout << "LE -> " << _return[0] << " | " << id << " | " << type << " | Not pending?" << std::endl;
    }
    catch (apache::thrift::transport::TTransportException e)
    {
      // std::cout << "Client read responses is FAILURE " << std::endl;
      (void)0;
    }
    *total_ += _return.size();
    _return.clear();
  }
}

void nln_client::read_responses()
{
  std::cout << "Client read responses is called " << std::endl;
  std::vector<std::string> _return;
  while (!done_->load())
  {
    auto type = requests_->pop();
    m_cond_->notify_one();
    try
    {

      // std::cout << "recv'd response?" << std::endl;

      int64_t id = reader_.recv_response(_return);
      auto found = pending_get_requests.find(id);
      if (found != pending_get_requests.end())

        std::cout << _return[0] << " | " << id << " | " << type << " | " << found->second[0] << " | " << _return.size() << " " << found->second.size() << std::endl;

      else

        std::cout << _return[0] << " | " << id << " | " << type << " | Not pending?" << std::endl;
    }
    catch (apache::thrift::transport::TTransportException e)
    {
      // std::cout << "Client read responses is FAILURE " << std::endl;
      (void)0;
    }
    *total_ += _return.size();
    _return.clear();
  }
}