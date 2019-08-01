#pragma once

#include "Result.h"

namespace douban {
namespace mc {
namespace keywords {

static const char kCR = '\r';
static const char kLF = '\n';
static const char kCRLF[] = "\r\n";
static const char kSPACE[] = " ";

static const char kGET[] = "get";
static const char kGETS[] = "gets";

static const char kSET_[] = "set ";
static const char kADD_[] = "add ";
static const char kREPLACE_[] = "replace ";
static const char kAPPEND_[] = "append ";
static const char kPREPEND_[] = "prepend ";
static const char kCAS_[] = "cas ";

static const char kDELETE_[] = "delete ";
static const char kTOUCH_[] = "touch ";

static const char kINCR_[] = "incr ";
static const char kDECR_[] = "decr ";

static const char k_NOREPLY[] = " noreply";

static const char kVERSION[] = "version";
static const char kSTATS[] = "stats";
static const char kFLUSHALL[] = "flush_all";

static const char kQUIT[] = "quit";


// error messages for human
static const char kSEND_ERROR[] = "send_error";
static const char kRECV_ERROR[] = "recv_error";
static const char kCONN_POLL_ERROR[] = "conn_poll_error";
static const char kPOLL_TIMEOUT_ERROR[] = "poll_timeout_error";
static const char kPOLL_ERROR[] = "poll_error";
static const char kSERVER_ERROR[] = "server_error";
static const char kPROGRAMMING_ERROR[] = "programming_error";
static const char kINVALID_KEY_ERROR[] = "invalid_key_error";
static const char kINCOMPLETE_BUFFER_ERROR[] = "incomplete_buffer_error";

static const char kUPDATE_SERVER[] = "update_server";
static const char kCONN_QUIT[] = "conn_quit";

} // namespace keywords
} // namespace mc
} // namespace douban
