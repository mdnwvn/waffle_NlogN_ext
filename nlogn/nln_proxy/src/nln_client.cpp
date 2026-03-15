

#include "nln_client.h"

#include "waffle/queue.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <iostream>

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
  response_thread_ = new std::thread(&nln_client::read_responses, this);

  // std::cout << "in constructor" << std::endl;
  //std::shared_ptr<TTransport> socket(new TSocket(host, port));

  // std::cout << "Socket created " << std::endl;
  //std::shared_ptr<TTransport> transport(new TFramedTransport(socket));
  //std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  // std::cout << "Storing pointer" << std::endl;

  // std::shared_ptr<waffle_thriftConcurrentClient> client_test_ = ;

  //transport->open();

  // std::string res;
  // std::string key = "user451800245123463256";
  // std:: cout << "getting key" << std::endl;
  // client_->get(res, key);

  std::cout << "Client created " << std::endl;
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

void nln_client::get_batch(const std::vector<std::string> &keys)
{
  std::cout << "client get_batch" << std::endl;
  std::vector<std::string> _return;
  seq_id_.__set_client_seq_no(sequence_num++);
  client_->async_get_batch(seq_id_, keys);

  requests_->push(GET_BATCH);
}

void nln_client::put_batch(const std::vector<std::string> &keys, const std::vector<std::string> &values)
{
  std::string _return;
  seq_id_.__set_client_seq_no(sequence_num++);
  client_->async_put_batch(seq_id_, keys, values);


  requests_->push(PUT_BATCH);
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

      std::cout << "recv'd response?" << std::endl;

      int64_t id = reader_.recv_response(_return);
      std::cout << _return[0] << " | " << id  << std::endl;
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