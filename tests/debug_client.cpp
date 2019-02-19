#include <unistd.h>
#include <iostream>
#include "Common.h"
#include "Client.h"
#include "test_common.h"

using douban::mc::Client;
using douban::mc::tests::newClient;


void set_(Client* client, const char* const* keys, const size_t* key_lens,
          const flags_t* flags, const char* const* vals, const size_t* val_len, size_t n = 1) {

  const exptime_t exptime = 0;
  const int noreply = 0;

  message_result_t **m_results = NULL;
  size_t nResults = 0;

  client->set(keys, key_lens, flags, exptime, NULL, noreply, vals, val_len,
              n, &m_results, &nResults);
  client->destroyMessageResult();
}


void get_(Client* client, const char* const * keys, const size_t* key_lens, size_t n = 1) {
  retrieval_result_t** r_results = NULL;
  size_t nResults = 0;
  client->get(keys, key_lens, n, &r_results, &nResults);
  client->destroyRetrievalResult();
}


int main() {
	Client* client = newClient(20);

	while (true) {

		const char* key = "hello";
		const size_t key_len = 5;

		const char* val = "world";
		const size_t val_len = 5;

		flags_t flags = 0;
		set_(client, &key, &key_len, &flags, &val, &val_len, 1);
		usleep(3000000);

		// get_(client, &key, &key_len, 1);

		retrieval_result_t** r_results = NULL;
		size_t nResults = 0;

		err_code_t rv = client->get(&key, &key_len, 1, &r_results, &nResults);
		client->destroyRetrievalResult();

		if (r_results!=NULL) {
			std::cout << r_results[0]->data_block;
		}

		std::cout << rv << std::endl;

	}
	delete client;

	return 0;
}
