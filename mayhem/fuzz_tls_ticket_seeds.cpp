// Parses a TLS ticket-seed file with TLSCredProcessor::processTLSTickets: either the plain JSON
// form, or the password-protected `openssl enc -aes-256-cbc -md sha256` form.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include <folly/FileUtil.h>
#include <folly/Optional.h>
#include <wangle/ssl/TLSCredProcessor.h>

#include "scratch_dir.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 1) {
    return -1;
  }
  const bool encrypted = data[0] & 1;
  ScratchDir dir;
  const std::string path = dir.file("seeds");
  if (!folly::writeFile(
          folly::StringPiece(reinterpret_cast<const char*>(data + 1), size - 1),
          path.c_str())) {
    std::abort();
  }
  folly::Optional<std::string> password;
  if (encrypted) {
    password = std::string("wangle-fuzz-password");
  }
  auto seeds = wangle::TLSCredProcessor::processTLSTickets(path, password);
  if (seeds) {
    volatile size_t n = seeds->oldSeeds.size() + seeds->currentSeeds.size() +
        seeds->newSeeds.size();
    (void)n;
  }
  return 0;
}
