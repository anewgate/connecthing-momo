#ifndef CONNECTHING_CLIENT_H_
#define CONNECTHING_CLIENT_H_

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <set>
#include <string>

// Boost
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/json.hpp>

#include "connecthing_data_channel_on_asio.h"
#include "metrics/stats_collector.h"
#include "rtc/rtc_manager.h"
#include "rtc/rtc_message_sender.h"
#include "url_parts.h"
#include "watchdog.h"
#include "websocket.h"

#define API_SECRET "janus-connecthing"
#define PING_INTERVAL 5

struct ConnecthingClientConfig {
  std::vector<std::string>
      signaling_urls;  // = { "wss://example.com/signaling" };
  std::string channel_id;

  bool insecure = false;
  bool video = true;
  bool audio = true;
  std::string video_codec_type = "";
  std::string audio_codec_type = "";
  int video_bit_rate = 0;
  int audio_bit_rate = 0;
  boost::json::value metadata;
  std::string role = "sendonly";
  bool spotlight = false;
  int spotlight_number = 0;
  int port = -1;
  bool simulcast = false;
  boost::optional<bool> data_channel_signaling;
  int data_channel_signaling_timeout = 180;
  boost::optional<bool> ignore_disconnect_websocket;
  int disconnect_wait_timeout = 5;
  std::string client_cert;
  std::string client_key;

  std::string proxy_url;
  std::string proxy_username;
  std::string proxy_password;
};

class ConnecthingClient
    : public std::enable_shared_from_this<ConnecthingClient>,
      public RTCMessageSender,
      public StatsCollector,
      public ConnecthingDataChannelObserver {
  ConnecthingClient(boost::asio::io_context& ioc,
                    RTCManager* manager,
                    ConnecthingClientConfig config);

 public:
  static std::shared_ptr<ConnecthingClient> Create(
      boost::asio::io_context& ioc,
      RTCManager* manager,
      ConnecthingClientConfig config) {
    return std::shared_ptr<ConnecthingClient>(
        new ConnecthingClient(ioc, manager, std::move(config)));
  }
  ~ConnecthingClient();
  void Close(std::function<void()> on_close);

  void Reset();
  void Connect();

  webrtc::PeerConnectionInterface::IceConnectionState GetRTCConnectionState()
      const;
  std::shared_ptr<RTCConnection> GetRTCConnection() const;

  void GetStats(std::function<void(
                    const webrtc::scoped_refptr<const webrtc::RTCStatsReport>&)>
                    callback) override;

 private:
  void ReconnectAfter();
  void OnWatchdogExpired();
  bool ParseURL(const std::string& url, URLParts& parts, bool& ssl) const;

 private:
  void DoRead();
  void DoSendConnect(bool redirect);
  void DoSendPong();
  void DoSendPong(
      const webrtc::scoped_refptr<const webrtc::RTCStatsReport>& report);
  void DoSendUpdate(const std::string& sdp, std::string type);
  //   std::shared_ptr<RTCConnection> CreateRTCConnection(
  //       boost::json::value jconfig);
  std::shared_ptr<RTCConnection> CreateRTCConnection();

 private:
  void OnConnect(boost::system::error_code ec,
                 std::string url,
                 std::shared_ptr<Websocket> ws);
  void OnRead(boost::system::error_code ec,
              std::size_t bytes_transferred,
              std::string text);

  void Redirect(std::string url);
  void OnRedirect(boost::system::error_code ec,
                  std::string url,
                  std::shared_ptr<Websocket> ws);

 private:
  webrtc::DataBuffer ConvertToDataBuffer(const std::string& label,
                                         const std::string& input);
  void SendDataChannel(const std::string& label, const std::string& input);

 private:
  // DataChannel 周りのコールバック
  void OnStateChange(webrtc::scoped_refptr<webrtc::DataChannelInterface>
                         data_channel) override;
  void OnMessage(
      webrtc::scoped_refptr<webrtc::DataChannelInterface> data_channel,
      const webrtc::DataBuffer& buffer) override;

 private:
  // WebRTC からのコールバック
  // これらは別スレッドからやってくるので取り扱い注意
  void OnIceConnectionStateChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
  void OnIceCandidate(const std::string sdp_mid,
                      const int sdp_mlineindex,
                      const std::string sdp) override;

 private:
  void DoIceConnectionStateChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state);

 private:
  boost::asio::io_context& ioc_;
  std::vector<std::shared_ptr<Websocket>> connecting_wss_;
  std::string connected_signaling_url_;
  std::shared_ptr<Websocket> ws_;
  std::shared_ptr<ConnecthingDataChannelOnAsio> dc_;
  //   bool using_datachannel_ = false;
  bool using_datachannel_ = true;
  std::set<std::string> compressed_labels_;

  std::atomic_bool destructed_ = {false};

  RTCManager* manager_;
  std::shared_ptr<RTCConnection> connection_;
  ConnecthingClientConfig config_;

  int retry_count_;
  webrtc::PeerConnectionInterface::IceConnectionState rtc_state_;

  WatchDog watchdog_;

 private:
  std::string transaction;
  int64_t session_id = -1;
  int64_t handle_id = -1;
  int64_t private_id = -1;
  boost::asio::deadline_timer timer_;
  void CreateSession();
  void AttachPlugin();
  void JoinRoomAsPublisher();
  void CreateOffer();
  void StartSendPing();
};

#endif  // CONNECTHING_CLIENT_H_
