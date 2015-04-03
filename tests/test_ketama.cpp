#include <stdio.h>
#include <fstream>
#include <string>

#include "Common.h"
#include "Connection.h"
#include "hashkit/ketama.h"
#include "gtest/gtest.h"
#include "test_common.h"

using std::atoi;
using std::string;
using std::getline;
using std::ifstream;
using std::stringstream;
using douban::mc::hashkit::KetamaSelector;
using douban::mc::tests::get_resource_path;
using douban::mc::Connection;

size_t wc(const char* path) {
  ifstream hintLineFile(path);
  size_t nLines = std::count(
      std::istreambuf_iterator<char>(hintLineFile),
      std::istreambuf_iterator<char>(),
      '\n'
  );
  hintLineFile.close();
  return nLines;
}


void load_servers(KetamaSelector& ks, Connection* conns, size_t nConns, const char* csv_path) {
  ifstream serverFile(csv_path);
  string line;
  ASSERT_TRUE(serverFile.good());
  int i = 0;
  while (std::getline(serverFile, line)) {
    stringstream lineStream(line);
    string hostname = "", port_;
    getline(lineStream, hostname, ',');
    getline(lineStream, port_, ',');
    uint32_t port = static_cast<uint32_t>(atoi(port_.c_str()));
    conns[i].init(hostname.c_str(), port);
    ++i;
  }
  ASSERT_EQ(nConns, i);
  ks.addServers(conns, i);

  serverFile.close();
}

void valid_key_pool(KetamaSelector& ks, const char* csv_path) {
  ifstream key_pool(csv_path);
  string line;
  ASSERT_TRUE(key_pool.good());
  while (std::getline(key_pool, line)) {
    stringstream lineStream(line);
    string key = "", idx_;
    getline(lineStream, key, ',');
    getline(lineStream, idx_, ',');
    int idx = atoi(idx_.c_str());
    bool check_alive = false;
    ASSERT_EQ(ks.getServer(key.c_str(), key.size(), check_alive), idx);
  }
  key_pool.close();
}


TEST(test_ketama, servers) {
  string csv_path = get_resource_path("server_port.csv");
  size_t nServers = wc(csv_path.c_str());
  ASSERT_EQ(nServers, 200);

  Connection* conns = new Connection[nServers];
  KetamaSelector ks;
  load_servers(ks, conns, nServers, csv_path.c_str());
  valid_key_pool(ks, get_resource_path("key_pool_idx.csv").c_str());
  delete[] conns;
}
