// Per-input scratch directory under $TMPDIR (mkdtemp); removed on destruction.
#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>

#include <unistd.h>

class ScratchDir {
 public:
  ScratchDir() {
    const char* base = std::getenv("TMPDIR");
    std::string tmpl = std::string(base && *base ? base : "/tmp") + "/wangle-fuzz-XXXXXX";
    if (mkdtemp(tmpl.data()) == nullptr) {
      std::abort();
    }
    path_ = tmpl;
  }
  ~ScratchDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }
  ScratchDir(const ScratchDir&) = delete;
  ScratchDir& operator=(const ScratchDir&) = delete;
  std::string file(const char* name) const {
    return path_ + "/" + name;
  }

 private:
  std::string path_;
};
