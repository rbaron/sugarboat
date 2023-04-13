#include <Arduino.h>

#include <string>

namespace sugarboat {

// Quick dirty hack for a Stream that writes to a string.
class StringStream : public Stream {
 public:
  int available() override {
    return str_.size() - ptr;
  }
  int read() {
    return available() ? str_[ptr++] : -1;
  }
  int peek() {
    return available() ? str_[ptr] : -1;
  }
  void flush() {}

  size_t write(uint8_t byte) override {
    // Very inefficient, but this is just a hack, although hopefully std::string
    // has a sensible strategy for mem allocation that amortizes this.
    str_ += byte;
    return 1;
  }

  const std::string& GetString() {
    return str_;
  }

 private:
  std::string str_;
  size_t ptr = 0;
};

}  // namespace sugarboat