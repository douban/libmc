#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <queue>

#include "Connection.h"
#include "Keywords.h"

using douban::mc::io::BufferWriter;
using douban::mc::io::BufferReader;

namespace douban {
namespace mc {

Connection::Connection()
    : m_counter(0), m_port(0), m_socketFd(-1),
      m_alive(false), m_hasAlias(false), m_deadUntil(0),
      m_connectTimeout(MC_DEFAULT_CONNECT_TIMEOUT),
      m_retryTimeout(MC_DEFAULT_RETRY_TIMEOUT),
      m_maxRetries(MC_DEFAULT_MAX_RETRIES), m_retires(0) {
  m_name[0] = '\0';
  m_host[0] = '\0';
  m_buffer_writer = new BufferWriter();
  m_buffer_reader = new BufferReader();
  m_parser.setBufferReader(m_buffer_reader);
}

Connection::Connection(const Connection& conn) {
  // never_called
}

Connection::~Connection() {
  this->close();
  delete m_buffer_writer;
  delete m_buffer_reader;
}

int Connection::init(const char* host, uint32_t port, const char* alias) {
  snprintf(m_host, sizeof m_host, "%s", host);
  m_port = port;
  if (alias == NULL) {
    m_hasAlias = false;
    snprintf(m_name, sizeof m_name, "%s:%u", m_host, m_port);
  } else {
    m_hasAlias = true;
    snprintf(m_name, sizeof m_name, "%s", alias);
  }
  return -1; // -1 means the connection is not established yet
}


int Connection::connect() {
  assert(!m_alive);
  this->close();
  struct addrinfo hints, *server_addrinfo = NULL, *ai_ptr = NULL;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  char str_port[MC_NI_MAXSERV] = "";
  snprintf(str_port, MC_NI_MAXSERV, "%u", m_port);
  if (getaddrinfo(m_host, str_port, &hints, &server_addrinfo) != 0) {
    if (server_addrinfo) {
      freeaddrinfo(server_addrinfo);
    }
    return -1;
  }

  int opt_nodelay = 1, opt_keepalive = 1;
  for (ai_ptr = server_addrinfo; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
    int fd = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
    if (fd == -1) {
      continue;
    }

    // non blocking
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
      goto try_next_ai;
    }

    if ((flags & O_NONBLOCK) == 0) {
      if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        goto try_next_ai;
      }
    }

    // no delay
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt_nodelay, sizeof opt_nodelay) != 0) {
      goto try_next_ai;
    }

    // keep alive
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt_keepalive, sizeof opt_keepalive) != 0) {
      goto try_next_ai;
    }

    // make sure the connection is established
    if (connectPoll(fd, ai_ptr) == 0) {
      m_socketFd = fd;
      m_alive = true;
      break;
    }

try_next_ai:
    ::close(fd);
    continue;
  }

  if (server_addrinfo) {
    freeaddrinfo(server_addrinfo);
  }
  return m_alive ? 0 : -1;
}

int Connection::connectPoll(int fd, struct addrinfo* ai_ptr) {
  int conn_rv = ::connect(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
  if (conn_rv == 0) {
    return 0;
  }
  assert(conn_rv == -1);
  switch (errno) {
    case EINPROGRESS:
    case EALREADY:
      {
        nfds_t n_fds = 1;
        pollfd_t pollfds[n_fds];
        pollfds[0].fd = fd;
        pollfds[0].events = POLLOUT;
        int max_timeout = 6;
        while (--max_timeout) {
          int poll_rv = poll(pollfds, n_fds, m_connectTimeout);
          if (poll_rv == 1) {
            if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
              return -1;
            } else {
              return 0;
            }
          } else if (poll_rv == -1) {
            return -1;
          }
        }
        return -1;
      }
    default:
      return -1;
  }
}

void Connection::close() {
  if (m_socketFd > 0) {
    m_alive = false;
    ::close(m_socketFd);
    m_socketFd = -1;
  }
}

bool Connection::tryReconnect(bool check_retries) {
  if (!m_alive) {
    if (check_retries) {
      m_retires += 1;
      if (m_retires > m_maxRetries) {
        return m_alive;
      }
    }
    time_t now;
    time(&now);
    if (now >= m_deadUntil) {
      int rv = this->connect();
      if (rv == 0) {
        if (m_deadUntil > 0) {
          log_info("Connection %s is back to live at %lu", m_name, now);
        }
        m_deadUntil = 0;
      } else {
        m_deadUntil = now + m_retryTimeout;
        // log_info("%s is still dead", m_name);
      }
    }
  }
  return m_alive;
}

void Connection::markDead(const char* reason, int delay) {
  if (m_alive) {
    time(&m_deadUntil);
    m_deadUntil += delay; // check after `delay` seconds, default 0
    this->close();
    if (strcmp(reason, keywords::kCONN_QUIT) != 0) {
      log_warn("Connection %s is dead(reason: %s, delay: %d), next check at %lu",
               m_name, reason, delay, m_deadUntil);
      struct iovec* key = m_parser.currentRequestKey();
      if (key != NULL) {
        log_warn("%s: first request key: %.*s", m_name,
                 static_cast<int>(key->iov_len),
                 static_cast<char*>(key->iov_base));
      }
    }
  }
}

int Connection::socketFd() const {
  return m_socketFd;
}

void Connection::takeBuffer(const char* const buf, size_t buf_len) {
  m_buffer_writer->takeBuffer(buf, buf_len);
}

void Connection::addRequestKey(const char* const key, const size_t len) {
  m_parser.addRequestKey(key, len);
}

size_t Connection::requestKeyCount() {
  return m_parser.requestKeyCount();
}

void Connection::setParserMode(ParserMode md) {
  m_parser.setMode(md);
}

void Connection::takeNumber(int64_t val) {
  m_buffer_writer->takeNumber(val);
}

ssize_t Connection::send() {
  struct msghdr msg = {};

  size_t n = 0;
  msg.msg_iov = const_cast<struct iovec *>(m_buffer_writer->getReadPtr(n));
  msg.msg_iovlen = n;

  // otherwise may lead to EMSGSIZE, SEE issue#3 on code
  int flags = 0;
  if (msg.msg_iovlen > MC_UIO_MAXIOV) {
    msg.msg_iovlen = MC_UIO_MAXIOV;
    flags = MC_MSG_MORE;
  }

  ssize_t nSent = ::sendmsg(m_socketFd, &msg, flags);

  if (nSent == -1) {
    return -1;
  } else {
    m_buffer_writer->commitRead(nSent);
  }

  size_t msgLeft = m_buffer_writer->msgIovlen();
  if (msgLeft == 0) {
    return 0;
  } else {
    return msgLeft;
  }
}

ssize_t Connection::recv(bool peek) {
  int flags = 0;
  size_t bufferSize = m_buffer_reader->getNextPreferedDataBlockSize();
  size_t bufferSizeAvailable = m_buffer_reader->prepareWriteBlock(bufferSize);
  char* writePtr = m_buffer_reader->getWritePtr();
  if (peek) {
    flags |= MSG_PEEK;
  }
  ssize_t bufferSizeActual = ::recv(m_socketFd, writePtr, bufferSizeAvailable, flags);
  // log_info("%p recv(%lu) %.*s", this, bufferSizeActual, (int)bufferSizeActual, writePtr);
  if (!peek && bufferSizeActual > 0) {
    m_buffer_reader->commitWrite(bufferSizeActual);
  }
  return bufferSizeActual;
}

void Connection::process(err_code_t& err) {
  return m_parser.process_packets(err);
}

types::RetrievalResultList* Connection::getRetrievalResults() {
  return m_parser.getRetrievalResults();
}

types::MessageResultList* Connection::getMessageResults() {
  return m_parser.getMessageResults();
}

types::LineResultList* Connection::getLineResults() {
  return m_parser.getLineResults();
}

types::UnsignedResultList* Connection::getUnsignedResults() {
  return m_parser.getUnsignedResults();
}

std::vector<struct iovec>* Connection::getRequestKeys() {
  return m_parser.getRequestKeys();
}

void Connection::reset() {
  m_counter = 0;
  m_retires = 0;
  m_parser.reset();
  m_buffer_reader->reset();
  m_buffer_writer->reset(); // flush data dispatched but not sent
}

void Connection::rewind() {
  m_parser.rewind();
  m_buffer_reader->reset(); // flush received data
  m_buffer_writer->rewind(); // rewind for resending data
}

void Connection::setRetryTimeout(int timeout) {
  m_retryTimeout = timeout;
}

void Connection::setConnectTimeout(int timeout) {
  m_connectTimeout = timeout;
}

void Connection::setMaxRetries(int max_retries) {
  m_maxRetries = max_retries;
}

} // namespace mc
} // namespace douban
