#pragma once

#include <sys/uio.h>
#include <queue>

#include "Common.h"
#include "BufferReader.h"
#include "Result.h"


namespace douban {
namespace mc {

typedef enum {
  MODE_UNDEFINED,
  MODE_END_STATE,
  MODE_COUNTING
} ParserMode;


class PacketParser {
 public:
  PacketParser();
  explicit PacketParser(io::BufferReader* reader);
  ~PacketParser();
  void setBufferReader(io::BufferReader* reader);
  void setMode(ParserMode md);
  void addRequestKey(const char* const key, const size_t len);
  std::queue<struct iovec>* getRequestKeys();
  size_t requestKeyCount();
  void process_packets(err_code_t &err);
  void reset();

  types::RetrievalResultList* getRetrievalResults();
  types::MessageResultList* getMessageResults();
  types::LineResultList* getLineResults();
  types::UnsignedResultList* getUnsignedResults();

 protected:
  int start_state(err_code_t& err);
  bool canEndParse();
  void processMessageResult(message_result_type tp);
  void processLineResult(err_code_t& err);


  std::queue<struct ::iovec> m_requestKeys;
  io::BufferReader* m_buffer_reader;
  parser_state_t m_state;
  ParserMode m_mode;
  size_t m_expectedResultCount;

  types::RetrievalResultList m_retrievalResults;
  types::MessageResultList m_messageResults;
  types::LineResultList m_lineResults;
  types::UnsignedResultList m_unsignedResults;

  // mt means Member-Tmp-variable
  types::RetrievalResult* mt_kvPtr;
};


inline bool PacketParser::canEndParse() {
  assert(m_mode == MODE_END_STATE || m_mode == MODE_COUNTING);
  return m_mode == MODE_END_STATE ? IS_END_STATE(m_state) : m_requestKeys.empty();
}

} // namespace mc
} // namespace douban
