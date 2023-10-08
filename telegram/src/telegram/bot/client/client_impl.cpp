#include <telegram/bot/client/client_impl.hpp>

#include <userver/formats/parse/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

ClientImpl::ClientImpl(clients::http::Client& http_client,
                       std::string bot_token,
                       std::string api_base_url,
                       std::string file_base_url)
    : http_client_(http_client),
      bot_token_(std::move(bot_token)),
      api_base_url_(std::move(api_base_url)),
      file_base_url_(std::move(file_base_url)) {}

CloseRequest ClientImpl::Close(const RequestOptions& request_options) {
  return FormRequest<CloseRequest>(CloseMethod::Parameters{}, request_options);
}

CopyMessageRequest ClientImpl::CopyMessage(
    const CopyMessageMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<CopyMessageRequest>(parameters, request_options);
}

ForwardMessageRequest ClientImpl::ForwardMessage(
    const ForwardMessageMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<ForwardMessageRequest>(parameters, request_options);
}

GetChatRequest ClientImpl::GetChat(
    const GetChatMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<GetChatRequest>(parameters, request_options);
}

GetFileRequest ClientImpl::GetFile(
    const GetFileMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<GetFileRequest>(parameters, request_options);
}

GetMeRequest ClientImpl::GetMe(const RequestOptions& request_options) {
  return FormRequest<GetMeRequest>(GetMeMethod::Parameters{}, request_options);
}

GetUpdatesRequest ClientImpl::GetUpdates(
    const GetUpdatesMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<GetUpdatesRequest>(parameters, request_options);
}

LogOutRequest ClientImpl::LogOut(const RequestOptions& request_options) {
  return FormRequest<LogOutRequest>(LogOutMethod::Parameters{},
                                    request_options);
}

SendAnimationRequest ClientImpl::SendAnimation(
    const SendAnimationMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendAnimationRequest>(parameters, request_options);
}

SendAudioRequest ClientImpl::SendAudio(
    const SendAudioMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendAudioRequest>(parameters, request_options);
}

SendChatActionRequest ClientImpl::SendChatAction(
    const SendChatActionMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendChatActionRequest>(parameters, request_options);
}

SendContactRequest ClientImpl::SendContact(
    const SendContactMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendContactRequest>(parameters, request_options);
}

SendDiceRequest ClientImpl::SendDice(
    const SendDiceMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendDiceRequest>(parameters, request_options);
}

SendDocumentRequest ClientImpl::SendDocument(
    const SendDocumentMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendDocumentRequest>(parameters, request_options);
}

SendLocationRequest ClientImpl::SendLocation(
    const SendLocationMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendLocationRequest>(parameters, request_options);
}

SendMessageRequest ClientImpl::SendMessage(
    const SendMessageMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendMessageRequest>(parameters, request_options);
}

SendPhotoRequest ClientImpl::SendPhoto(
    const SendPhotoMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendPhotoRequest>(parameters, request_options);
}

SendPollRequest ClientImpl::SendPoll(
    const SendPollMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendPollRequest>(parameters, request_options);
}

SendVenueRequest ClientImpl::SendVenue(
    const SendVenueMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendVenueRequest>(parameters, request_options);
}

SendVideoNoteRequest ClientImpl::SendVideoNote(
    const SendVideoNoteMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendVideoNoteRequest>(parameters, request_options);
}

SendVideoRequest ClientImpl::SendVideo(
    const SendVideoMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendVideoRequest>(parameters, request_options);
}

SendVoiceRequest ClientImpl::SendVoice(
    const SendVoiceMethod::Parameters& parameters,
    const RequestOptions& request_options) {
  return FormRequest<SendVoiceRequest>(parameters, request_options);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
