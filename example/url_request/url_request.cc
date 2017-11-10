#include "example/url_request/url_request_context_getter.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "base/timer/timer.h"
#include "base/bind.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "url/gurl.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/http/http_network_session.h"
#include "net/http/http_network_layer.h"
#include <iostream>
#include <string>

class SyncUrlFetcher : public net::URLFetcherDelegate {
 public:
  SyncUrlFetcher(const GURL& url,
                 URLRequestContextGetter* getter,
                 std::string* response)
      : url_(url),
        getter_(getter),
        response_(response),
        event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
               base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  ~SyncUrlFetcher() override {}

  bool Fetch() {
    getter_->GetNetworkTaskRunner()->PostTask(
        FROM_HERE,
        base::Bind(&SyncUrlFetcher::FetchOnIOThread, base::Unretained(this)));
    event_.Wait();
    return success_;
  }

  void FetchOnIOThread() {
    fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this);
    fetcher_->SetRequestContext(getter_);
    fetcher_->Start();
  }

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    success_ = (source->GetResponseCode() == 200);
    if (success_)
      success_ = source->GetResponseAsString(response_);
    fetcher_.reset();  // Destroy the fetcher on IO thread.
    event_.Signal();
  }

 private:
  GURL url_;
  URLRequestContextGetter* getter_;
  std::string* response_;
  base::WaitableEvent event_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  bool success_;
};

bool FetchUrl(const std::string& url,
              URLRequestContextGetter* getter,
              std::string* response) {
  return SyncUrlFetcher(GURL(url), getter, response).Fetch();
}

int main(int argc, const char* const* argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

  base::Thread io_thread("io thread");
  base::Thread::Options options(base::MessageLoop::TYPE_IO, 0);
  CHECK(io_thread.StartWithOptions(options));
  URLRequestContextGetter* context_getter = new URLRequestContextGetter(io_thread.task_runner());

  std::string response;
  FetchUrl("http://www.sojson.com/open/api/weather/json.shtml?city=%E6%88%90%E9%83%BD", context_getter, &response);

  std::cout << "url response:" << response << std::endl;

  while (1) {
    Sleep(1000);
  }
  
  system("pause");
}