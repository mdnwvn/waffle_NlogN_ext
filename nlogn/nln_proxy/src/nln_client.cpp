

#include "nln_client.h"

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
  //std::cout << "in constructor" << std::endl;
  std::shared_ptr<TTransport> socket(new TSocket(host, port));

  //std::cout << "Socket created " << std::endl;
  std::shared_ptr<TTransport> transport(new TFramedTransport(socket));
  std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));


  //std::cout << "Storing pointer" << std::endl;

  //std::shared_ptr<waffle_thriftConcurrentClient> client_test_ = ;

  client_ = std::make_shared<waffle_thriftConcurrentClient>(protocol);

  transport->open();

  std::string res;
  std::string key = "user451800245123463256";
  std:: cout << "getting key" << std::endl;
  client_->get(res, key);

  std::cout << "Client created | " <<  res << std::endl;
}

std::string nln_client::get_level(const std::string &key)
{
  std::string res;
  client_->get(res, key);
  return res;
}

std::vector<std::string> nln_client::get_batch(const std::vector<std::string> &keys)
{
  std::vector<std::string> _return;
  client_->get_batch(_return, keys);
  return _return;
}

void nln_client::put_batch(const std::vector<std::string> &keys, const std::vector<std::string> &values)
{
  std::string _return;
  client_->put_batch(keys, values);
}
