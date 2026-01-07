#ifndef CONNECTHING_SERVER_H_
#define CONNECTHING_SERVER_H_

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

// Boost
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "connecthing_client.h"
#include "rtc/rtc_manager.h"

struct ConnecthingServerConfig {};

class ConnecthingServer
    : public std::enable_shared_from_this<ConnecthingServer> {
  ConnecthingServer(boost::asio::io_context& ioc,
                    boost::asio::ip::tcp::endpoint endpoint,
                    std::shared_ptr<ConnecthingClient> client,
                    RTCManager* rtc_manager,
                    ConnecthingServerConfig config);

 public:
  static std::shared_ptr<ConnecthingServer> Create(
      boost::asio::io_context& ioc,
      boost::asio::ip::tcp::endpoint endpoint,
      std::shared_ptr<ConnecthingClient> client,
      RTCManager* rtc_manager,
      ConnecthingServerConfig config) {
    return std::shared_ptr<ConnecthingServer>(new ConnecthingServer(
        ioc, endpoint, client, rtc_manager, std::move(config)));
  }

  void Run();

 private:
  void DoAccept();
  void OnAccept(boost::system::error_code ec);

 private:
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;

  std::shared_ptr<ConnecthingClient> client_;
  RTCManager* rtc_manager_;
  ConnecthingServerConfig config_;
};

#endif
