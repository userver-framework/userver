#pragma once

#include <userver/telegram/bot/client/client.hpp>

#include <userver/clients/http/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

class ClientImpl : public Client {
public:
  ClientImpl(clients::http::Client& http_client,
             std::string bot_token,
             std::string api_base_url,
             std::string file_base_url);

  CloseRequest Close(const RequestOptions& request_options) override;

  CopyMessageRequest CopyMessage(
      const CopyMessageMethod::Parameters& parameters,
      const RequestOptions& request_options) override;
  
  ForwardMessageRequest ForwardMessage(
      const ForwardMessageMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  GetChatRequest GetChat(
      const GetChatMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  GetFileRequest GetFile(
      const GetFileMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  GetMeRequest GetMe(const RequestOptions& request_options) override;

  GetUpdatesRequest GetUpdates(
      const GetUpdatesMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  LogOutRequest LogOut(const RequestOptions& request_options) override;

  SendAnimationRequest SendAnimation(
      const SendAnimationMethod::Parameters& parameters,
      const RequestOptions& request_options) override;
  
  SendAudioRequest SendAudio(
      const SendAudioMethod::Parameters& parameters,
      const RequestOptions& request_options) override;
  
  SendChatActionRequest SendChatAction(
      const SendChatActionMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendContactRequest SendContact(
      const SendContactMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendDiceRequest SendDice(
      const SendDiceMethod::Parameters& parameters,
      const RequestOptions& request_options) override;
  
  SendDocumentRequest SendDocument(
      const SendDocumentMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendLocationRequest SendLocation(
      const SendLocationMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendMessageRequest SendMessage(
      const SendMessageMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendPhotoRequest SendPhoto(
      const SendPhotoMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendPollRequest SendPoll(
      const SendPollMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendVenueRequest SendVenue(
      const SendVenueMethod::Parameters& parameters,
      const RequestOptions& request_options) override;
  
  SendVideoNoteRequest SendVideoNote(
      const SendVideoNoteMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendVideoRequest SendVideo(
      const SendVideoMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

  SendVoiceRequest SendVoice(
      const SendVoiceMethod::Parameters& parameters,
      const RequestOptions& request_options) override;

private:
  template <typename Request>
  Request FormRequest(
      const typename Request::Method::Parameters& parameters,
      const RequestOptions& request_options) {
    return Request(http_client_.CreateRequest(),
                   api_base_url_,
                   bot_token_,
                   request_options,
                   parameters);
  }

  clients::http::Client& http_client_;
  const std::string bot_token_;
  const std::string api_base_url_;
  const std::string file_base_url_;
};

}  // namespace telegram::bot

USERVER_NAMESPACE_END
