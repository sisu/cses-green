#include "gen-cpp/Judge.h"

namespace cses {
namespace protocol {

class Judge: public cses::protocol::JudgeIf {
public:
  virtual bool hasFile(const std::string& token, const std::string& hash) override;
  virtual void sendFile(const std::string& token, const std::string& data) override;
  virtual void getFile(std::string& _return, const std::string& token, const std::string& hash) override;
  virtual void run(RunResult& _return, const std::string& token, const std::string& image, const std::vector<FileRef> & inputs, const RunOptions& options) override;
};

}
}
