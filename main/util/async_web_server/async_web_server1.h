#pragma once

#include <esp_http_server.h>

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <optional>

namespace esp32
{

  class AsyncWebParameter
  {
  public:
    AsyncWebParameter(const std::string &value) : value_(value) {}
    const std::string &value() const { return this->value_; }

  protected:
    std::string value_;
  };

  class AsyncWebServerRequest;

  class AsyncResponse
  {
  public:
    AsyncResponse(const AsyncWebServerRequest *req) : req_(req)
    {
    }
    virtual ~AsyncResponse() {}

    void addHeader(const char *name, const char *value);

    virtual const char *get_content_data() const = 0;
    virtual size_t get_content_size() const = 0;

  protected:
    const AsyncWebServerRequest *req_;
  };

  class AsyncEmptyResponse : public AsyncResponse
  {
  public:
    AsyncEmptyResponse(const AsyncWebServerRequest *req) : AsyncResponse(req)
    {
    }

    const char *get_content_data() const override { return nullptr; };
    size_t get_content_size() const override { return 0; };
  };

  class AsyncWebServerResponse : public AsyncResponse
  {
  public:
    AsyncWebServerResponse(const AsyncWebServerRequest *req, const std::string &content)
        : AsyncResponse(req), content_(content)
    {
    }

    const char *get_content_data() const override
    {
      return this->content_.c_str();
    };

    size_t get_content_size() const override
    {
      return this->content_.size();
    };

  protected:
    std::string content_;
  };

  class AsyncResponseStream : public AsyncResponse
  {
  public:
    AsyncResponseStream(const AsyncWebServerRequest *req) : AsyncResponse(req) {}

    const char *get_content_data() const override
    {
      return this->content_.c_str();
    };

    size_t get_content_size() const override
    {
      return this->content_.size();
    };

    void print(const char *str)
    {
      this->content_.append(str);
    }

    void print(const std::string &str)
    {
      this->content_.append(str);
    }

    void print(float value);
    void printf(const char *fmt, ...) __attribute__((format(printf, 2, 3)));

  protected:
    std::string content_;
  };

  class AsyncWebServerRequest
  {
    friend class AsyncWebServerResponse;
    friend class AsyncWebServer;

  public:
    ~AsyncWebServerRequest();

    http_method method() const
    {
      return static_cast<http_method>(this->req_->method);
    }

    std::string url() const;
    std::string host() const;
    size_t contentLength() const { return this->req_->content_len; }

    void redirect(const std::string &url);

    void send(AsyncResponse *response);
    void send(int code, const char *content_type = nullptr, const char *content = nullptr);
    AsyncEmptyResponse *beginResponse(int code, const char *content_type)
    {
      auto *res = new AsyncEmptyResponse(this); // NOLINT(cppcoreguidelines-owning-memory)
      this->init_response_(res, 200, content_type);
      return res;
    }
    // NOLINTNEXTLINE(readability-identifier-naming)
    AsyncWebServerResponse *beginResponse(int code, const char *content_type, const std::string &content)
    {
      auto *res = new AsyncWebServerResponse(this, content); // NOLINT(cppcoreguidelines-owning-memory)
      this->init_response_(res, code, content_type);
      return res;
    }

    AsyncResponseStream *beginResponseStream(const char *content_type)
    {
      auto *res = new AsyncResponseStream(this); // NOLINT(cppcoreguidelines-owning-memory)
      this->init_response_(res, 200, content_type);
      return res;
    }

    bool hasParam(const std::string &name) { return this->getParam(name) != nullptr; }
    AsyncWebParameter *getParam(const std::string &name);

    bool hasArg(const char *name) { return this->hasParam(name); }
    std::string arg(const std::string &name)
    {
      auto *param = this->getParam(name);
      if (param)
      {
        return param->value();
      }
      return {};
    }

    operator httpd_req_t *() const { return this->req_; }
    std::optional<std::string> get_header(const char *name) const;

  protected:
    httpd_req_t *req_;
    AsyncResponse *rsp_{};
    std::map<std::string, AsyncWebParameter *> params_;
    AsyncWebServerRequest(httpd_req_t *req) : req_(req) {}
    void init_response_(AsyncResponse *rsp, int code, const char *content_type);
  };

  class AsyncWebHandler;

  
  class AsyncWebHandler
  {
  public:
    virtual ~AsyncWebHandler() = default;
    virtual bool canHandle(AsyncWebServerRequest *request) { return false; }
    virtual void handleRequest(AsyncWebServerRequest *request) {}
    virtual void handleUpload(AsyncWebServerRequest *request, const std::string &filename, size_t index, uint8_t *data,
                              size_t len, bool final) {}
    virtual void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {}
    virtual bool isRequestHandlerTrivial() { return true; }
  };

  typedef uint8_t WebRequestMethodComposite;
  typedef std::function<void(AsyncWebServerRequest *request)> ArRequestHandlerFunction;
  typedef std::function<void(AsyncWebServerRequest *request, const std::string &filename, size_t index, uint8_t *data, size_t len, bool final)>
      ArUploadHandlerFunction;
  typedef std::function<void(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)>
      ArBodyHandlerFunction;

  class AsyncCallbackWebHandler : public AsyncWebHandler
  {
  private:
  protected:
    std::string _uri;
    WebRequestMethodComposite _method;
    ArRequestHandlerFunction _onRequest;
    ArUploadHandlerFunction _onUpload;
    ArBodyHandlerFunction _onBody;

  public:
    AsyncCallbackWebHandler() : _uri(), _method(HTTP_ANY), _onRequest(NULL), _onUpload(NULL), _onBody(NULL)
    {
    }

    /////////////////////////////////////////////////

    inline void setUri(const std::string &uri)
    {
      _uri = uri;
    }

    /////////////////////////////////////////////////

    inline void setMethod(WebRequestMethodComposite method)
    {
      _method = method;
    }

    /////////////////////////////////////////////////

    inline void onRequest(ArRequestHandlerFunction fn)
    {
      _onRequest = fn;
    }

    /////////////////////////////////////////////////

    inline void onUpload(ArUploadHandlerFunction fn)
    {
      _onUpload = fn;
    }

    /////////////////////////////////////////////////

    inline void onBody(ArBodyHandlerFunction fn)
    {
      _onBody = fn;
    }

    /////////////////////////////////////////////////

    virtual bool canHandle(AsyncWebServerRequest *request) override final
    {
      if (!_onRequest)
        return false;

      if (!(_method & request->method()))
        return false;

      if (_uri.length() && _uri.startsWith("/*."))
      {
        std::string uriTemplate = std::string(_uri);
        uriTemplate = uriTemplate.substr(uriTemplate.find_last_of("."));

        if (!request->url().endsWith(uriTemplate))
          return false;
      }
      else if (_uri.length() && _uri.endsWith("*"))
      {
        std::string uriTemplate = std::string(_uri);
        uriTemplate = uriTemplate.substr(0, uriTemplate.length() - 1);

        if (!request->url().startsWith(uriTemplate))
          return false;
      }
      else if (_uri.length() && (_uri != request->url() && !request->url().startsWith(_uri + "/")))
        return false;

      request->addInterestingHeader("ANY");

      return true;
    }

    /////////////////////////////////////////////////

    virtual void handleRequest(AsyncWebServerRequest *request) override final
    {
      if (_onRequest)
        _onRequest(request);
      else
        request->send(500);
    }

    /////////////////////////////////////////////////

    virtual void handleUpload(AsyncWebServerRequest *request, const std::string &filename, size_t index, uint8_t *data,
                              size_t len, bool final) override final
    {
      if (_onUpload)
        _onUpload(request, filename, index, data, len, final);
    }

    /////////////////////////////////////////////////

    virtual void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index,
                            size_t total) override final
    {

      if (_onBody)
        _onBody(request, data, len, index, total);
    }

    /////////////////////////////////////////////////

    virtual bool isRequestHandlerTrivial() override final
    {
      return _onRequest ? false : true;
    }
  };

  class AsyncEventSource;

  class AsyncEventSourceResponse
  {
    friend class AsyncEventSource;

  public:
    void send(const char *message, const char *event = nullptr, uint32_t id = 0, uint32_t reconnect = 0);

  protected:
    AsyncEventSourceResponse(const AsyncWebServerRequest *request, AsyncEventSource *server);
    static void destroy(void *p);
    AsyncEventSource *server_;
    httpd_handle_t hd_{};
    int fd_{};
  };

  using AsyncEventSourceClient = AsyncEventSourceResponse;

  class AsyncEventSource : public AsyncWebHandler
  {
    friend class AsyncEventSourceResponse;
    using connect_handler_t = std::function<void(AsyncEventSourceClient *)>;

  public:
    AsyncEventSource(const std::string &url) : url_(url) {}
    ~AsyncEventSource() override;

    bool canHandle(AsyncWebServerRequest *request) override
    {
      return request->method() == HTTP_GET && request->url() == this->url_;
    }
    void handleRequest(AsyncWebServerRequest *request) override;
    void onConnect(connect_handler_t cb) { this->on_connect_ = cb; }

    void send(const char *message, const char *event = nullptr, uint32_t id = 0, uint32_t reconnect = 0);

  protected:
    std::string url_;
    std::set<AsyncEventSourceResponse *> sessions_;
    connect_handler_t on_connect_{};
  };

  class DefaultHeaders
  {
    friend class AsyncWebServerRequest;

  public:
    void addHeader(const char *name, const char *value) { this->headers_.emplace_back(name, value); }

    static DefaultHeaders &Instance()
    {
      static DefaultHeaders instance;
      return instance;
    }

  protected:
    std::vector<std::pair<std::string, std::string>> headers_;
  };

} // namespace esphome
