#include "MsQuicServer.h"
#include <utility>
#if __has_include(<msquic.h>)
#define SECOND_SCREEN_HAS_MSQUIC 1
#include <msquic.h>
#else
#define SECOND_SCREEN_HAS_MSQUIC 0
#endif
namespace second_screen {
struct MsQuicServer::Impl {
    Config config; quic::TransportCallbacks callbacks;
    bool running{false}; bool connected{false}; uint16_t maxSendLength{1200};
#if SECOND_SCREEN_HAS_MSQUIC
    const QUIC_API_TABLE* api{nullptr};
    HQUIC registration{nullptr}; HQUIC configuration{nullptr};
    HQUIC listener{nullptr}; HQUIC connection{nullptr}; HQUIC controlStream{nullptr};
#endif
};
MsQuicServer::MsQuicServer(Config config, quic::TransportCallbacks callbacks): impl_(std::make_unique<Impl>()) {
    impl_->config=std::move(config); impl_->callbacks=std::move(callbacks);
}
MsQuicServer::~MsQuicServer(){ stop(); }
bool MsQuicServer::start() {
#if !SECOND_SCREEN_HAS_MSQUIC
    return false;
#else
    return false;
#endif
}
void MsQuicServer::stop() {
    impl_->connected=false; impl_->running=false;
#if SECOND_SCREEN_HAS_MSQUIC
    impl_->controlStream=nullptr; impl_->connection=nullptr; impl_->listener=nullptr;
    impl_->configuration=nullptr; impl_->registration=nullptr; impl_->api=nullptr;
#endif
}
bool MsQuicServer::sendControl(const uint8_t*, size_t){ return false; }
bool MsQuicServer::sendVideoDatagram(const uint8_t* data,size_t size){
    if(!impl_->connected || !data || size>impl_->maxSendLength) return false; return false;
}
bool MsQuicServer::connected() const{return impl_->connected;}
quic::DatagramLimits MsQuicServer::datagramLimits() const{return {impl_->maxSendLength};}
}
