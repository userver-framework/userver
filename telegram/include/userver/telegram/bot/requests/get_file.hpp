#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/file.hpp>

#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to get basic information about a file and prepare
/// it for downloading.
/// @note For the moment, bots can download files of up to 20MB in size.
/// @note The file can then be downloaded via DownloadFile. The file will be
/// available by this file_path within one hour. After an hour, you can request
/// a new one by calling GetFile again.
/// @note This function may not preserve the original file name and MIME type.
/// You should save the file's MIME type and name (if available) when the File
/// object is received.
/// @see https://core.telegram.org/bots/api#getfile
struct GetFileMethod {
  static constexpr std::string_view kName = "getFile";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kGet;

  struct Parameters{
    Parameters(std::string _file_id);

    /// @brief File identifier to get information about
    std::string file_id;
  };

  /// @brief On success, a File object is returned.
  using Reply = File;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

GetFileMethod::Parameters Parse(const formats::json::Value& json,
                                formats::parse::To<GetFileMethod::Parameters>);

formats::json::Value Serialize(const GetFileMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using GetFileRequest = Request<GetFileMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
