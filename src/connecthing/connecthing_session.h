#ifndef CONNECTHING_SESSION_H_
#define CONNECTHING_SESSION_H_

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

// Boost
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/json.hpp>

#include "connecthing_client.h"
#include "rtc/rtc_manager.h"

struct ConnecthingSessionConfig {};

// HTTP の１回のリクエストに対して答えるためのクラス
class ConnecthingSession
    : public std::enable_shared_from_this<ConnecthingSession> {
  ConnecthingSession(boost::asio::ip::tcp::socket socket,
                     std::shared_ptr<ConnecthingClient> client,
                     RTCManager* rtc_manager,
                     ConnecthingSessionConfig config);

 public:
  static std::shared_ptr<ConnecthingSession> Create(
      boost::asio::ip::tcp::socket socket,
      std::shared_ptr<ConnecthingClient> client,
      RTCManager* rtc_manager,
      ConnecthingSessionConfig config) {
    return std::shared_ptr<ConnecthingSession>(new ConnecthingSession(
        std::move(socket), client, rtc_manager, std::move(config)));
  }

  void Run();

 private:
  void DoRead();

  void OnRead(boost::system::error_code ec, std::size_t bytes_transferred);
  void OnWrite(boost::system::error_code ec,
               std::size_t bytes_transferred,
               bool close);
  void DoClose();

 private:
  static boost::beast::http::response<boost::beast::http::string_body>
  CreateOKWithJSON(
      const boost::beast::http::request<boost::beast::http::string_body>& req,
      boost::json::value json_message);

  template <class Body, class Fields>
  void SendResponse(boost::beast::http::response<Body, Fields> msg) {
    auto sp = std::make_shared<boost::beast::http::response<Body, Fields>>(
        std::move(msg));

    // msg オブジェクトは書き込みが完了するまで生きている必要があるので、
    // メンバに入れてライフタイムを延ばしてやる
    res_ = sp;

    // Write the response
    boost::beast::http::async_write(
        socket_, *sp,
        std::bind(&ConnecthingSession::OnWrite, shared_from_this(),
                  std::placeholders::_1, std::placeholders::_2,
                  sp->need_eof()));
  }

 private:
  boost::asio::ip::tcp::socket socket_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::request<boost::beast::http::string_body> req_;
  std::shared_ptr<void> res_;

  std::shared_ptr<ConnecthingClient> client_;
  RTCManager* rtc_manager_;
  ConnecthingSessionConfig config_;
};

#endif  // CONNECTHING_SESSION_H_
