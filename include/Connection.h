#pragma once

#include <netdb.h>
#include <stdint.h>
#include <ctime>

#include <queue>

#include "Common.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Parser.h"
#include "Result.h"


namespace douban {
namespace mc {

class Connection {

 public:
    Connection();
    ~Connection();
    int init(const char* host, uint32_t port, uint32_t weight = 0);
    int connect();
    void close();
    const bool alive();
    bool tryReconnect();
    void markDead(const char* reason, int delay);
    int socketFd() const;

    const char* name();
    const char* host();
    const uint32_t port();
    const uint32_t weight();

    void takeBuffer(const char* const buf, size_t buf_len);
    void addRequestKey(const char* const key, const size_t len);
    size_t requestKeyCount();
    void setParserMode(ParserMode md);
    void takeNumber(int64_t val);
    ssize_t send();
    ssize_t recv();
    void process(err_code_t& err);
    types::RetrievalResultList* getRetrievalResults();
    types::MessageResultList* getMessageResults();
    types::LineResultList* getLineResults();
    types::UnsignedResultList* getUnsignedResults();

    std::queue<struct iovec>* getRequestKeys();


    void reset();
    static void setRetryTimeout(int timeout);
    static const int getRetryTimeout();
    static void setConnectTimeout(int timeout);

    size_t m_counter;

 protected:
    int connectPoll(int fd, struct addrinfo* ai_ptr);

    char m_name[MC_NI_MAXHOST + 1 + MC_NI_MAXSERV];
    char m_host[MC_NI_MAXHOST];
    uint32_t m_port;
    uint32_t m_weight; // default 0: without weight

    int m_socketFd;
    bool m_alive;
    time_t m_deadUntil;
    io::BufferWriter* m_buffer_writer; // for send
    io::BufferReader* m_buffer_reader; // for recv
    PacketParser m_parser;

    static int s_retryTimeout;
    static int s_connectTimeout;

 private:
    Connection(const Connection& conn);
};


inline const bool Connection::alive() {
  return m_alive;
}


inline const char* Connection::name() {
  return m_name;
}

inline const char* Connection::host() {
  return m_host;
}

inline const uint32_t Connection::port() {
  return m_port;
}


inline const uint32_t Connection::weight() {
  return m_weight;
}


inline const int Connection::getRetryTimeout() {
  return s_retryTimeout;
}


} // namespace mc
} // namespace douban
