#include "Client.h"
#include "test_common.h"

#include <cassert>
#include "gtest/gtest.h"

using douban::mc::Client;
using douban::mc::tests::newUnixClient;

TEST(test_unix, establish_connection) {
  Client* client = newUnixClient();
  EXPECT_TRUE(client != NULL);
  delete client;
}