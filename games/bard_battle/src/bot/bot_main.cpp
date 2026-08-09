#include "net/client.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

int main(int argc,char ** argv) try {
    std::string server="127.0.0.1";std::size_t count=1;
    for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg=="--server"&&i+1<argc)server=argv[++i];else if(arg=="--count"&&i+1<argc)count=std::stoul(argv[++i]);}
    count=std::min(count,net::kMaxPlayers);std::vector<std::unique_ptr<net::Client>> clients;
    for(std::size_t i=0;i<count;++i){auto client=std::make_unique<net::Client>();client->connect(server,"Bot-"+std::to_string(i+1));clients.push_back(std::move(client));}
    std::uint64_t frame=0;while(true){for(std::size_t i=0;i<clients.size();++i){auto & client=*clients[i];client.update();const float phase=static_cast<float>((frame+i*30)%240);const std::int8_t x=phase<120?100:-100;const std::int8_t y=((frame/120+i)%2)==0?60:-60;client.send_input(x,y);if(frame%30==0&&client.connected())client.send_rhythm_hit(0,{1.0f,0.0f},net::RhythmAction::shoot);}if(frame%300==0){std::size_t connected=0;for(const auto & c:clients)connected+=c->connected();std::cout<<connected<<"/"<<clients.size()<<" bots connected\n";}++frame;std::this_thread::sleep_for(std::chrono::milliseconds(16));}
}catch(const std::exception & e){std::cerr<<"Bot error: "<<e.what()<<'\n';return 1;}
