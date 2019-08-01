#include "Client.h"
#include "Result.h"
#include "test_common.h"

#include <cstring>
#include "gtest/gtest.h"

using douban::mc::Client;
using douban::mc::io::DataBlock;
using douban::mc::tests::newClient;

void hint() {
  fprintf(stderr, "ConnectionTimeoutError!\nPlease`./misc/memcached_server restart` manually.\n");
  EXPECT_EQ(1, 0);
}

TEST(test_client, del_get_set_get_del) {
  DataBlock::setMinCapacity(4);
  Client* client = newClient(1);
  if (client == NULL) {
    hint();
  } else {
    retrieval_result_t **r_results = NULL;
    message_result_t **m_results = NULL;
    size_t nResults = 0;
    flags_t flags[] = {0, 0, 0};
    size_t key_lens[] = {3, 6, 5};
    exptime_t exptime = 0;

    const char* keys[] = {
      "foo", "tuiche", "buzai"
    };

    const char* vals[] = {
      "value of foo", "value of tuiche", "value of buzai"
    };
    size_t val_lens[] = {12, 15, 14};


    client->_delete(keys, key_lens, 0, 3, &m_results, &nResults);
    client->destroyMessageResult();

    client->get(keys, key_lens, 3, &r_results, &nResults);
    EXPECT_EQ(nResults, 0);
    client->destroyRetrievalResult();

    client->set(keys, key_lens, flags, exptime, NULL, 0, vals, val_lens, 3, &m_results, &nResults);
    for (size_t i = 0; i < nResults; i++) {
      message_result_t* r = m_results[i];
      ASSERT_TRUE(strncmp(r->key, keys[0], key_lens[0]) == 0 ||
          strncmp(r->key, keys[1], key_lens[1]) == 0 ||
          strncmp(r->key, keys[2], key_lens[2]) == 0);
      ASSERT_EQ(r->type_, MSG_STORED);
    }
    client->destroyMessageResult();

    client->get(keys, key_lens, 3, &r_results, &nResults);
    EXPECT_EQ(nResults, 3);
    for (size_t i = 0; i < 3; i++) {
      ASSERT_EQ(r_results[i]->bytes, val_lens[i]);
      ASSERT_N_STREQ(r_results[i]->data_block, vals[i], val_lens[i]);
    }
    client->destroyRetrievalResult();

    client->_delete(keys, key_lens, 0, 3, &m_results, &nResults);
    for (size_t i = 0; i < nResults; i++) {
      message_result_t* r = m_results[i];
      ASSERT_TRUE(strncmp(r->key, keys[0], key_lens[0]) == 0 ||
          strncmp(r->key, keys[1], key_lens[1]) == 0 ||
          strncmp(r->key, keys[2], key_lens[2]) == 0);
      ASSERT_EQ(r->type_, MSG_DELETED);
    }
    ASSERT_EQ(nResults, 3);
    client->destroyMessageResult();
    delete client;
  }
}


TEST(test_client, test_storage) {
  Client* client = newClient(1);
  if (client == NULL) {
    hint();
  } else {
    const char* keys[] = {
      "foo", "tuiche", "buzai"
    };


    const char* vals[] = {
      "value of foo", "value of tuiche", "value of buzai"
    };
    size_t val_lens[] = {12, 15, 14};

    retrieval_result_t **r_results = NULL;
    message_result_t **m_results = NULL;
    size_t key_lens[] = {3, 6, 5};
    flags_t flags[] = {0, 0, 0};
    exptime_t exptime = 0;
    size_t nResults = 0;


    client->_delete(keys, key_lens, 0, 3, &m_results, &nResults);
    client->destroyMessageResult();

    // set foo
    client->set(keys, key_lens, flags, exptime, NULL, 0, vals, val_lens, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_STORED);
    client->destroyMessageResult();

    // add tuiche
    client->add(keys + 1, key_lens + 1, flags + 1, exptime, NULL, 0,
        vals + 1, val_lens + 1, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_STORED);
    client->destroyMessageResult();

    // add tuiche
    client->add(keys + 1, key_lens + 1, flags + 1, exptime, NULL, 0,
        vals + 1, val_lens + 1, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_NOT_STORED);
    client->destroyMessageResult();

    // replace tuiche
    client->replace(keys + 1, key_lens + 1, flags + 1, exptime, NULL, 0,
        vals + 1, val_lens + 1, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_STORED);
    client->destroyMessageResult();

    // replace buzai
    client->replace(keys + 2, key_lens + 2, flags + 2, exptime, NULL, 0,
        vals + 2, val_lens + 2, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_NOT_STORED);
    client->destroyMessageResult();

    // prepend foo with value in tuiche
    client->prepend(keys, key_lens, flags, exptime, NULL, 0,
        vals + 1, val_lens + 1, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_STORED);
    client->destroyMessageResult();

    client->get(keys, key_lens, 1, &r_results, &nResults);
    EXPECT_EQ(nResults, 1);

    ASSERT_EQ(r_results[0]->bytes, val_lens[0] + val_lens[1]);
    ASSERT_N_STREQ(r_results[0]->data_block, vals[1], val_lens[1]);
    ASSERT_N_STREQ(r_results[0]->data_block + val_lens[1], vals[0], val_lens[0]);
    client->destroyRetrievalResult();

    // append tuiche with value in buzai
    client->append(keys + 1, key_lens + 1, flags + 1, exptime, NULL, 0,
        vals + 2, val_lens + 2, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_STORED);
    client->destroyMessageResult();

    client->get(keys + 1, key_lens + 1, 1, &r_results, &nResults);
    EXPECT_EQ(nResults, 1);

    ASSERT_EQ(r_results[0]->bytes, val_lens[1] + val_lens[2]);
    ASSERT_N_STREQ(r_results[0]->data_block, vals[1], val_lens[1]);
    ASSERT_N_STREQ(r_results[0]->data_block + val_lens[1], vals[2], val_lens[2]);
    client->destroyRetrievalResult();

    // prepend buzai
    client->prepend(keys + 2, key_lens + 2, flags + 2, exptime, NULL, 0,
        vals + 2, val_lens + 2, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_NOT_STORED);
    client->destroyMessageResult();

    // append buzai
    client->append(keys + 2, key_lens + 2, flags + 2, exptime, NULL, 0,
        vals + 2, val_lens + 2, 1, &m_results, &nResults);
    EXPECT_EQ(nResults, 1);
    ASSERT_EQ(m_results[0]->type_, MSG_NOT_STORED);
    client->destroyMessageResult();

    delete client;
  }
}

TEST(test_client, test_version) {
  Client* client = newClient(1);
  if (client == NULL) {
    hint();
  } else {
    broadcast_result_t* results = NULL;
    size_t nHosts = 0;
    client->version(&results, &nHosts);
    for (size_t i = 0; i < nHosts; i++) {
      broadcast_result_t* r = results + i;
      ASSERT_TRUE(r->len == 1);
      ASSERT_TRUE(r->line_lens[0]);
      // possible version strings:
      // - "1.4.14 (Ubuntu)" (on Travis-CI)
      // - "1.5.2"
      // First char is expected to be a digit
      char c = r->lines[0][0];
      ASSERT_TRUE('0' <= c && c <= '9');
    }
    client->destroyBroadcastResult();
    delete client;
  }
}

TEST(test_client, test_flush_all) {
  Client* client = newClient(2);
  if (client == NULL) {
    hint();
  } else {
    broadcast_result_t* results = NULL;
    size_t nHosts = 0;
    client->flushAll(&results, &nHosts);
    // The feature is disabled by default,
    // so no hosts are flushed
    ASSERT_EQ(nHosts, 0);

    client->toggleFlushAllFeature(true);
    client->flushAll(&results, &nHosts);
    ASSERT_EQ(nHosts, 2);

    for (size_t i = 0; i < nHosts; i++) {
      broadcast_result_t* r = results + i;
      ASSERT_TRUE(r->msg_type == MSG_OK);
    }
    client->destroyBroadcastResult();


    client->toggleFlushAllFeature(false);
    client->flushAll(&results, &nHosts);
    ASSERT_EQ(nHosts, 0);
    delete client;
  }
}


TEST(test_client, test_stats) {
  Client* client = newClient(1);
  if (client == NULL) {
    hint();
  } else {
    broadcast_result_t* results = NULL;
    size_t nHosts = 0;
    client->stats(&results, &nHosts);
    for (size_t i = 0; i < nHosts; i++) {
      broadcast_result_t* r = results + i;
      ASSERT_TRUE(strlen(r->host) > 0);
      // printf("host: %s\n", r->host);
      ASSERT_TRUE(r->len > 10);
      // for (int j = 0; j < r->len; j++) {
      //   printf("\t[%.*s]\n", static_cast<int>(r->line_lens[j]), r->lines[j]);
      // }
      // printf("\r\n");
    }
    client->destroyBroadcastResult();
    delete client;
  }
}


TEST(test_client, test_touch) {
  Client* client = newClient(1);
  if (client == NULL) {
    hint();
  } else {
    message_result_t **m_results = NULL;
    size_t nResults = 0;
    exptime_t exptime = 0;

    const char* keys[] = {
      "foo", "tuiche", "buzai"
    };

    size_t key_lens[] = {3, 6, 5};

    flags_t flags[] = {0, 0, 0};

    const char* vals[] = {
      "value of foo", "value of tuiche", "value of buzai"
    };
    size_t val_lens[] = {12, 15, 14};

    client->_delete(keys, key_lens, 0, 3, &m_results, &nResults);
    client->destroyMessageResult();

    client->set(keys, key_lens, flags, exptime, NULL, 0, vals, val_lens, 1, &m_results, &nResults);
    client->destroyMessageResult();

    client->touch(keys, key_lens, exptime, 0, 3, &m_results, &nResults);
    EXPECT_EQ(nResults, 3);
    for (size_t i = 0; i < nResults; i++) {
      message_result_t* r = m_results[i];
      if (strncmp(r->key, keys[0], key_lens[0]) == 0) {
        ASSERT_EQ(r->type_, MSG_TOUCHED);
      } else {
        ASSERT_EQ(r->type_, MSG_NOT_FOUND);
      }
    }
    client->destroyMessageResult();

    delete client;
  }
}


TEST(test_client, test_incr_decr) {
  Client* client = newClient(1);
  if (client == NULL) {
    hint();
  } else {
    const char* keys[] = {
      "foo", "tuiche", "buzai"
    };

    size_t key_lens[] = {3, 6, 5};

    flags_t flags[] = {0, 0, 0};

    const char* vals[] = {
      "99", "101", "100"
    };
    uint64_t deltas[] = {
      1, 1, 1
    };
    size_t val_lens[] = {2, 3, 3};

    message_result_t **m_results = NULL;
    unsigned_result_t *u_results = NULL;

    size_t nResults = 0;
    exptime_t exptime = 0;

    client->_delete(keys, key_lens, 0, 3, &m_results, &nResults);
    client->destroyMessageResult();

    client->set(keys, key_lens, flags, exptime, NULL, 0, vals, val_lens,
        2, &m_results, &nResults);
    client->destroyMessageResult();

    client->incr(keys[0], key_lens[0], deltas[0], 0, &u_results, &nResults);
    ASSERT_EQ(1, nResults);
    client->destroyUnsignedResult();
    client->incr(keys[0], key_lens[0], deltas[0], 0, &u_results, &nResults);
    ASSERT_EQ(1, nResults);
    client->destroyUnsignedResult();
    client->incr(keys[0], key_lens[0], deltas[0], 0, &u_results, &nResults);
    ASSERT_EQ(1, nResults);
    client->destroyUnsignedResult();

    client->decr(keys[0], key_lens[0], deltas[0], 0, &u_results, &nResults);
    ASSERT_EQ(1, nResults);
    client->destroyUnsignedResult();
    client->decr(keys[0], key_lens[0], deltas[0], 0, &u_results, &nResults);
    ASSERT_EQ(1, nResults);
    client->destroyUnsignedResult();
    ASSERT_EQ(1, nResults);
    client->decr(keys[0], key_lens[0], deltas[0], 0, &u_results, &nResults);
    client->destroyUnsignedResult();

    delete client;
  }
}


TEST(client, noreply) {
  Client* client = newClient(1);
  if (client == NULL) {
    hint();
  } else {
    const char* keys[] = {
      "foo", "tuiche", "buzai"
    };

    size_t key_lens[] = {3, 6, 5};

    message_result_t **m_results = NULL;

    size_t nResults = 0;
    bool noreply = true;

    client->_delete(keys, key_lens, noreply, 3, &m_results, &nResults);
    client->destroyMessageResult();
    delete client;
  }
}

TEST(client, update_server) {
  Client* client = newClient(3);
  if (client == NULL) {
    hint();
  } else {
    const char * hosts[] = {
        "127.0.0.1",
        "127.0.0.1",
        "127.0.0.1",
    };
    const uint32_t ports[] = {
        21211, 21212, 11213
    };
    const char * aliases[] = {
        "alfa",
        "bravo",
        "charlie"
    };
    int rv;

    rv = client->updateServers(hosts, ports, 2, aliases);
    ASSERT_EQ(rv, 1);

    const char * mismatch_aliases[] = {
        "afla",
        "bravo",
        "charlie"
    };
    rv = client->updateServers(hosts, ports, 3, mismatch_aliases);
    ASSERT_EQ(rv, 2);

    rv = client->updateServers(hosts, ports, 3, aliases);
    ASSERT_EQ(rv, -3);
  }
}
