// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "AFesqueSvc.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

class AFesqueSvcHandler : virtual public AFesqueSvcIf {
 public:
  AFesqueSvcHandler() {
    // Your initialization goes here
  }

  void Fetch(std::string& _return, const std::string& path) {
    // Your implementation goes here
    printf("Fetch\n");
  }

  void Fetch_Chunk(std::string& _return, const std::string& path, const int32_t chunk) {
    // Your implementation goes here
    printf("Fetch_Chunk\n");
  }

  void Store(FStatShort& _return, const std::string& path, const std::string& file_str) {
    // Your implementation goes here
    printf("Store\n");
  }

  void Remove(const std::string& path) {
    // Your implementation goes here
    printf("Remove\n");
  }

  void Create(const std::string& path, const int32_t mode) {
    // Your implementation goes here
    printf("Create\n");
  }

  void MakeDir(const std::string& path, const int32_t mode) {
    // Your implementation goes here
    printf("MakeDir\n");
  }

  void RemoveDir(const std::string& path) {
    // Your implementation goes here
    printf("RemoveDir\n");
  }

  void TestAuth(FStatShort& _return, const std::string& path) {
    // Your implementation goes here
    printf("TestAuth\n");
  }

  void GetFileStat(FStat& _return, const std::string& path) {
    // Your implementation goes here
    printf("GetFileStat\n");
  }

  void ListDir(std::vector<tDirent> & _return, const std::string& path) {
    // Your implementation goes here
    printf("ListDir\n");
  }

  void server_test(const std::string& msg, const int32_t delay) {
    // Your implementation goes here
    printf("server_test\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  ::std::shared_ptr<AFesqueSvcHandler> handler(new AFesqueSvcHandler());
  ::std::shared_ptr<TProcessor> processor(new AFesqueSvcProcessor(handler));
  ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

