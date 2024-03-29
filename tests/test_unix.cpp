#include "Common.h"
#include "Client.h"
#include "test_common.h"

#include <cassert>
#include <sys/stat.h>
#include "gtest/gtest.h"

using douban::mc::Client;
using douban::mc::tests::md5Client;
using douban::mc::splitServerString;

Client* newUnixClient() {
  const char * hosts[] = { "/tmp/env_mc_dev/var/run/unix_test.socket" };
  const uint32_t ports[] = { 0 };
  struct stat info;
  // fails if ../misc/memcached_server wasn't started with startall or unix
  EXPECT_EQ(stat(hosts[0], &info), 0);

  return md5Client(hosts, ports, 1);
}

TEST(test_unix, establish_connection) {
  Client* client = newUnixClient();
  ASSERT_TRUE(client != NULL);
  delete client;
}

TEST(test_unix, host_parse_regression) {
  char test[] = "127.0.0.1:21211 testing";
  server_string_split_t out = splitServerString(test);
  ASSERT_STREQ(out.host, "127.0.0.1");
  ASSERT_STREQ(out.port, "21211");
  ASSERT_STREQ(out.alias, "testing");
}

TEST(test_unix, socket_path_spaces) {
  char test[] = "/tmp/spacey\\ path testing";
  server_string_split_t out = splitServerString(test);
  ASSERT_STREQ(out.host, "/tmp/spacey\\ path");
  ASSERT_EQ(out.port, nullptr);
  ASSERT_STREQ(out.alias, "testing");
}

TEST(test_unix, socket_path_escaping) {
  char test[] = "/tmp/spicy\\\\ path testing";
  server_string_split_t out = splitServerString(test);
  ASSERT_STREQ(out.host, "/tmp/spicy\\\\");
  ASSERT_EQ(out.port, nullptr);
  ASSERT_STREQ(out.alias, "path");
}

TEST(test_unix, alias_space_escaping) {
  char test[] = "/tmp/path testing\\ alias";
  server_string_split_t out = splitServerString(test);
  ASSERT_STREQ(out.host, "/tmp/path");
  ASSERT_EQ(out.port, nullptr);
  ASSERT_STREQ(out.alias, "testing\\ alias");
}
