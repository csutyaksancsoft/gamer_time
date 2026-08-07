#include "net/client.h"

#include "core/error.h"

#include <algorithm>
#include <mutex>
#include <cstdlib>

namespace net {

namespace {
std::mutex g_enet_mutex;
std::uint32_t g_enet_users = 0;

void acquire_enet() {
    std::lock_guard lock(g_enet_mutex);
    if (g_enet_users++ == 0 && enet_initialize() != 0) {
        g_enet_users = 0;
        fail("ENet initialization failed");
    }
}

void release_enet() {
    std::lock_guard lock(g_enet_mutex);
    if (g_enet_users > 0 && --g_enet_users == 0) enet_deinitialize();
}

std::pair<std::string, std::uint16_t> split_endpoint(const std::string & endpoint) {
    const auto colon = endpoint.rfind(':');
    if (colon == std::string::npos) return {endpoint, kDefaultPort};
    const int port = std::stoi(endpoint.substr(colon + 1));
    if (port < 1 || port > 65535) fail("Invalid server port");
    return {endpoint.substr(0, colon), static_cast<std::uint16_t>(port)};
}
}

Client::~Client() { disconnect(); }

void Client::connect(const std::string & endpoint, const std::string & name) {
    disconnect();
    acquire_enet();
    enet_initialized_ = true;
    host_ = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!host_) fail("Failed to create ENet client");
    const auto [hostname, port] = split_endpoint(endpoint);
    if (enet_address_set_host(&address_, hostname.c_str()) != 0) fail("Could not resolve server: " + hostname);
    address_.port = port;
    name_ = name;
    reconnect_deadline_us_ = monotonic_time_us() + 15000000ULL;
    begin_connection();
}

void Client::begin_connection() {
    peer_ = enet_host_connect(host_, &address_, 2, kProtocolVersion);
    if (!peer_) fail("No ENet peer available");
    status_ = "connecting";
}

void Client::disconnect() {
    if (peer_) enet_peer_disconnect_now(peer_, 0);
    if (host_) enet_host_destroy(host_);
    host_ = nullptr; peer_ = nullptr; connected_ = false; player_id_ = 0;
    if (enet_initialized_) { release_enet(); enet_initialized_ = false; }
}

void Client::send(const std::vector<std::uint8_t> & bytes, bool reliable) {
    if (!peer_ || !connected_) return;
    ENetPacket * packet = enet_packet_create(bytes.data(), bytes.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(peer_, reliable ? 0 : 1, packet);
}

void Client::update() {
    if (!host_) return;
    ENetEvent event{};
    while (enet_host_service(host_, &event, 0) > 0) {
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            connected_ = true; status_ = "connected"; send(make_hello(name_), true);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            connected_ = false; player_id_ = 0; status_ = "reconnecting"; peer_ = nullptr; next_reconnect_us_ = monotonic_time_us() + 500000ULL;
        } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            try { receive({event.packet->data, event.packet->dataLength}); } catch (...) { enet_packet_destroy(event.packet); throw; }
            enet_packet_destroy(event.packet);
        }
    }
    const auto now = monotonic_time_us();
    if (!connected_ && !peer_ && now >= next_reconnect_us_ && now < reconnect_deadline_us_) begin_connection();
    if (!connected_ && !peer_ && now >= reconnect_deadline_us_) status_ = "reconnect timed out";
    if (connected_ && now - last_ping_us_ >= 500000) { last_ping_us_ = now; send(make_clock_ping(now), false); }
    enet_host_flush(host_);
}

void Client::receive(std::span<const std::uint8_t> bytes) {
    Reader reader(bytes);
    switch (reader.type()) {
    case MessageType::welcome:
        player_id_ = reader.u32(); spawn_ = {reader.f32(), reader.f32()}; status_ = "ready"; send(make_ready(), true); break;
    case MessageType::snapshot: snapshot_ = read_snapshot(reader); break;
    case MessageType::clock_pong: {
        const std::uint64_t client_send = reader.u64(); const std::uint64_t server_receive = reader.u64(); const std::uint64_t server_send = reader.u64(); const std::uint64_t client_receive = monotonic_time_us();
        const std::uint64_t rtt = (client_receive - client_send) - (server_send - server_receive);
        const std::int64_t offset = (static_cast<std::int64_t>(server_receive) - static_cast<std::int64_t>(client_send) + static_cast<std::int64_t>(server_send) - static_cast<std::int64_t>(client_receive)) / 2;
        if (rtt <= best_rtt_us_) { best_rtt_us_ = rtt; server_offset_us_ = offset; }
        break;
    }
    case MessageType::song_schedule: schedule_ = read_song_schedule(reader); has_schedule_ = true; break;
    case MessageType::rhythm_result: rhythm_results_.push_back(read_rhythm_result(reader)); break;
    default: break;
    }
}

void Client::send_input(std::int8_t x, std::int8_t y) { send(make_input(++input_sequence_, x, y, monotonic_time_us()), false); }
void Client::send_rhythm_hit(std::int16_t calibration_ms) { send(make_rhythm_hit(++rhythm_sequence_, server_time_us(), calibration_ms), true); }
bool Client::take_song_schedule(SongSchedule & schedule) { if(!has_schedule_) return false; schedule=schedule_; has_schedule_=false; return true; }
bool Client::take_rhythm_result(RhythmResult & result) { if(rhythm_results_.empty()) return false; result=rhythm_results_.front(); rhythm_results_.pop_front(); return true; }

} // namespace net
