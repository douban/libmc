#include "c_client.h"
#include "Client.h"


using douban::mc::Client;


void* client_create() {
  return new Client();
}


void client_init(void* client, const char* const * hosts, const uint32_t* ports,
                 size_t n, const char* const * aliases, const int failover) {
  douban::mc::Client* c = static_cast<Client*>(client);
  c->config(douban::mc::CFG_HASH_FUNCTION, douban::mc::OPT_HASH_CRC_32); // TODO
  c->init(hosts, ports, n, aliases);
  if (failover) {
    c->enableConsistentFailover();
  } else {
    c->disableConsistentFailover();
  }
}


void client_destroy(void* client) {
  douban::mc::Client* c = static_cast<Client*>(client);
  delete c;
}


int client_version(void* client, broadcast_result_t** results, size_t* nHosts) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->version(results, nHosts);
}


void client_destroy_broadcast_result(void* client) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->destroyBroadcastResult();
}


int client_get(void* client, const char* const* keys, const size_t* keyLens,
               size_t nKeys, retrieval_result_t*** results, size_t* nResults) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->get(keys, keyLens, nKeys, results, nResults);
}

void client_destroy_retrieval_result(void* client) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->destroyRetrievalResult();
}


int client_set(void* client, const char* const* keys, const size_t* key_lens,
               const flags_t* flags, const exptime_t exptime,
               const cas_unique_t* cas_uniques, const bool noreply,
               const char* const* vals, const size_t* val_lens,
               size_t nItems, message_result_t*** results, size_t* nResults) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->set(keys, key_lens, flags, exptime, cas_uniques,
                noreply, vals, val_lens, nItems, results, nResults);
}


void client_destroy_message_result(void* client) {
  douban::mc::Client* c = static_cast<Client*>(client);
  return c->destroyMessageResult();
}
