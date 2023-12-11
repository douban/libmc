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
    int init(const char* host, uint32_t port, const char* alias = NULL);
    int connect();
    void close();
    const bool alive();
    bool tryReconnect(bool check_retries = true);
    void markDead(const char* reason, int delay = 0);
    int socketFd() const;

    const char* name();
    const char* host();
    const uint32_t port();
    const bool hasAlias();
    const bool isSent();

    void takeBuffer(const char* const buf, size_t buf_len);
    void addRequestKey(const char* const key, const size_t len);
    size_t requestKeyCount();
    void setParserMode(ParserMode md);
    void takeNumber(int64_t val);
    ssize_t send();
    ssize_t recv(bool peek = false);
    void process(err_code_t& err);
    types::RetrievalResultList* getRetrievalResults();
    types::MessageResultList* getMessageResults();
    types::LineResultList* getLineResults();
    types::UnsignedResultList* getUnsignedResults();

    std::vector<struct iovec>* getRequestKeys();


    void reset();
    void rewind();
    void setRetryTimeout(int timeout);
    const int getRetryTimeout();
    void setConnectTimeout(int timeout);
    void setMaxRetries(int max_retries);

    size_t m_counter;

 protected:
    int connectPoll(int fd, const sockaddr* ai_ptr, const socklen_t ai_addrlen);
    int unixSocketConnect();

    char m_name[MC_NI_MAXHOST + 1 + MC_NI_MAXSERV];
    char m_host[MC_NI_MAXHOST];
    uint32_t m_port;

    int m_socketFd;
    bool m_alive;
    bool m_hasAlias;
    bool m_unixSocket;
    time_t m_deadUntil;
    io::BufferWriter* m_buffer_writer; // for send
    io::BufferReader* m_buffer_reader; // for recv
    PacketParser m_parser;

    int m_connectTimeout;
    int m_retryTimeout;

    int m_maxRetries; // max reconnect tries during one command
    int m_retires;

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

inline const bool Connection::hasAlias() {
  return m_hasAlias;
}

inline const bool Connection::isSent() {
  return m_buffer_writer->isRead();
}

inline const int Connection::getRetryTimeout() {
  return m_retryTimeout;
}


} // namespace mc
} // namespace douban
