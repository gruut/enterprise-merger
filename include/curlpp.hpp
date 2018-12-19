#ifndef CURLPP_HPP
#define CURLPP_HPP

#include <cstddef>
#include <initializer_list>
#include <ostream>
#include <stdexcept>
#include <string>
#include <curl/curl.h>

namespace curlpp
{
  void init(long flags = CURL_GLOBAL_DEFAULT);
  void cleanup();

  namespace util
  {
    std::string escape(const std::string& string);
    std::string unescape(const std::string& string);
  }

  namespace write
  {
    std::size_t none(char*, std::size_t size, std::size_t nmemb, void*);
    std::size_t toStream(char* data, std::size_t size, std::size_t nmemb, std::ostream* stream);
    std::size_t toString(char* data, std::size_t size, std::size_t nmemb, std::string* string);
  }

  // Easy
  class Easy
  {
  public:
    typedef CURLcode codeType;
    typedef CURL handleType;
    typedef CURLINFO infoType;
    typedef CURLoption optionType;

    Easy();
    ~Easy();

    Easy(const Easy&) = delete;
    Easy& operator=(const Easy&) = delete;

    handleType* getHandle();

    std::string escape(std::string string);
    std::string unescape(std::string string);

    void pause(int bitmask);
    void perform();
    void reset();

    template <typename T>
    void getInfo(infoType info, T arg);

    template <typename T>
    void setOpt(optionType option, T arg);

  private:
    handleType* handle_ = nullptr;
  };

  // Form
  class Form
  {
  public:
    typedef CURLFORMcode codeType;
    typedef curl_httppost handleType;

    Form() = default;
    ~Form();

    Form(const Form&) = delete;
    Form& operator=(const Form&) = delete;

    handleType* getHandle() const;

    void addContent(const std::string& name, const std::string& value);
    void addContent(const std::string& name, const std::string& value, const std::string& type);

    void addFile(const std::string& name, const std::string& filename);
    void addFile(const std::string& name, const std::string& filename, const std::string& type);

  private:
    handleType* last_ = nullptr;
    handleType* post_ = nullptr;
  };

  // List
  class List
  {
  public:
    typedef curl_slist handleType;

    List() = default;
    List(std::initializer_list<std::string> fields);
    ~List();

    List(const List&) = delete;
    List& operator=(const List&) = delete;

    handleType* getHandle() const;

    void add(const std::string& field);

  private:
    mutable handleType* list_ = nullptr;
  };


  // Exception
  class Exception : public std::runtime_error
  {
  public:
    Exception(const std::string& message);
  };

  // EasyException
  class EasyException : public Exception
  {
  public:
    EasyException(Easy::codeType errorId);

    Easy::codeType getErrorId() const;

  private:
    const Easy::codeType errorId_;
  };

  // FormException
  class FormException : public Exception
  {
  public:
    FormException(Form::codeType errorId);

    Form::codeType getErrorId() const;

  private:
    const Form::codeType errorId_;
  };
}

// general
inline void curlpp::init(long flags)
{
  curl_global_init(flags);
}

inline void curlpp::cleanup()
{
  curl_global_cleanup();
}

// util
inline std::string curlpp::util::escape(const std::string& string)
{
  static Easy easy;
  return easy.escape(string);
}

inline std::string curlpp::util::unescape(const std::string& string)
{
  static Easy easy;
  return easy.unescape(string);
}

// write
inline std::size_t curlpp::write::none(char*, std::size_t size, std::size_t nmemb, void*)
{
  return size * nmemb;
}

inline std::size_t curlpp::write::toStream(char* data, std::size_t size, std::size_t nmemb, std::ostream* stream)
{
  if (!stream)
  {
    return 0;
  }

  stream->write(data, size * nmemb);
  stream->flush();

  return size * nmemb;
}

inline std::size_t curlpp::write::toString(char* data, std::size_t size, std::size_t nmemb, std::string* string)
{
  if (!string)
  {
    return 0;
  }

  string->append(data, size * nmemb);

  return size * nmemb;
}

// Easy
inline curlpp::Easy::Easy()
        : handle_(curl_easy_init())
{
  setOpt(CURLOPT_WRITEFUNCTION, write::none);
}

inline curlpp::Easy::~Easy()
{
  curl_easy_cleanup(handle_);
}

inline curlpp::Easy::handleType* curlpp::Easy::getHandle()
{
  return handle_;
}

inline std::string curlpp::Easy::escape(std::string string)
{
  char* ret = curl_easy_escape(handle_, string.c_str(), string.size());

  string = std::string(ret);

  curl_free(ret);

  return string;
}

inline std::string curlpp::Easy::unescape(std::string string)
{
  int out = 0;
  char* ret = curl_easy_unescape(handle_, string.c_str(), string.size(), &out);

  string = std::string(ret, out);

  curl_free(ret);

  return string;
}

inline void curlpp::Easy::pause(int bitmask)
{
  const codeType error = curl_easy_pause(handle_, bitmask);

  if (error != CURLE_OK)
  {
    throw EasyException(error);
  }
}

inline void curlpp::Easy::perform()
{
  const codeType error = curl_easy_perform(handle_);

  if (error != CURLE_OK)
  {
    throw EasyException(error);
  }
}

inline void curlpp::Easy::reset()
{
  curl_easy_reset(handle_);
}

template <typename T>
inline void curlpp::Easy::getInfo(infoType info, T arg)
{
  const codeType error = curl_easy_getinfo(handle_, info, arg);

  if (error != CURLE_OK)
  {
    throw EasyException(error);
  }
}

template <typename T>
inline void curlpp::Easy::setOpt(optionType option, T arg)
{
  const codeType error = curl_easy_setopt(handle_, option, arg);

  if (error != CURLE_OK)
  {
    throw EasyException(error);
  }
}

// Form
inline curlpp::Form::~Form()
{
  curl_formfree(post_);
}

inline curlpp::Form::handleType* curlpp::Form::getHandle() const
{
  return post_;
}

inline void curlpp::Form::addContent(const std::string& name, const std::string& value)
{
  const codeType error = curl_formadd(
          &post_,
          &last_,
          CURLFORM_COPYNAME,
          name.c_str(),
          CURLFORM_COPYCONTENTS,
          value.c_str(),
          CURLFORM_END
  );

  if (error != CURL_FORMADD_OK)
  {
    throw FormException(error);
  }
}

inline void curlpp::Form::addContent(const std::string& name, const std::string& value, const std::string& type)
{
  const codeType error = curl_formadd(
          &post_,
          &last_,
          CURLFORM_COPYNAME,
          name.c_str(),
          CURLFORM_COPYCONTENTS,
          value.c_str(),
          CURLFORM_CONTENTTYPE,
          type.c_str(),
          CURLFORM_END
  );

  if (error != CURL_FORMADD_OK)
  {
    throw FormException(error);
  }
}

inline void curlpp::Form::addFile(const std::string& name, const std::string& filename)
{
  const codeType error = curl_formadd(
          &post_,
          &last_,
          CURLFORM_COPYNAME,
          name.c_str(),
          CURLFORM_FILE,
          filename.c_str(),
          CURLFORM_END
  );

  if (error != CURL_FORMADD_OK)
  {
    throw FormException(error);
  }
}

inline void curlpp::Form::addFile(const std::string& name, const std::string& filename, const std::string& type)
{
  const codeType error = curl_formadd(
          &post_,
          &last_,
          CURLFORM_COPYNAME,
          name.c_str(),
          CURLFORM_FILE,
          filename.c_str(),
          CURLFORM_CONTENTTYPE,
          type.c_str(),
          CURLFORM_END
  );

  if (error != CURL_FORMADD_OK)
  {
    throw FormException(error);
  }
}

// List
inline curlpp::List::List(std::initializer_list<std::string> fields)
{
  for (const std::string& field : fields)
  {
    add(field);
  }
}

inline curlpp::List::~List()
{
  curl_slist_free_all(list_);
}

inline curlpp::List::handleType* curlpp::List::getHandle() const
{
  return list_;
}

inline void curlpp::List::add(const std::string& field)
{
  list_ = curl_slist_append(list_, field.c_str());
}

// Exception
inline curlpp::Exception::Exception(const std::string& message)
        : std::runtime_error(message)
{}

// EasyException
inline curlpp::EasyException::EasyException(Easy::codeType errorId)
        : Exception(curl_easy_strerror(errorId)), errorId_(errorId)
{}

inline curlpp::Easy::codeType curlpp::EasyException::getErrorId() const
{
  return errorId_;
}

// FormException
inline curlpp::FormException::FormException(Form::codeType errorId)
        : Exception("Form error"), errorId_(errorId)
{}

inline curlpp::Form::codeType curlpp::FormException::getErrorId() const
{
  return errorId_;
}

#endif
