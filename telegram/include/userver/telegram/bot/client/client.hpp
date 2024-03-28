#pragma once

#include <userver/telegram/bot/requests/close.hpp>
#include <userver/telegram/bot/requests/copy_message.hpp>
#include <userver/telegram/bot/requests/forward_message.hpp>
#include <userver/telegram/bot/requests/get_chat.hpp>
#include <userver/telegram/bot/requests/get_file.hpp>
#include <userver/telegram/bot/requests/get_me.hpp>
#include <userver/telegram/bot/requests/get_updates.hpp>
#include <userver/telegram/bot/requests/log_out.hpp>
#include <userver/telegram/bot/requests/send_animation.hpp>
#include <userver/telegram/bot/requests/send_audio.hpp>
#include <userver/telegram/bot/requests/send_chat_action.hpp>
#include <userver/telegram/bot/requests/send_contact.hpp>
#include <userver/telegram/bot/requests/send_dice.hpp>
#include <userver/telegram/bot/requests/send_document.hpp>
#include <userver/telegram/bot/requests/send_location.hpp>
#include <userver/telegram/bot/requests/send_message.hpp>
#include <userver/telegram/bot/requests/send_photo.hpp>
#include <userver/telegram/bot/requests/send_poll.hpp>
#include <userver/telegram/bot/requests/send_venue.hpp>
#include <userver/telegram/bot/requests/send_video_note.hpp>
#include <userver/telegram/bot/requests/send_video.hpp>
#include <userver/telegram/bot/requests/send_voice.hpp>

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

class Client {
 public:
  ~Client() = default;

  virtual CloseRequest Close(const RequestOptions& request_options) = 0;

  virtual CopyMessageRequest CopyMessage(
      const CopyMessageMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;
  
  virtual ForwardMessageRequest ForwardMessage(
      const ForwardMessageMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;
  
  virtual GetChatRequest GetChat(
      const GetChatMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual GetFileRequest GetFile(
      const GetFileMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual GetMeRequest GetMe(const RequestOptions& request_options) = 0;

  virtual GetUpdatesRequest GetUpdates(
      const GetUpdatesMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual LogOutRequest LogOut(const RequestOptions& request_options) = 0;

  virtual SendAnimationRequest SendAnimation(
      const SendAnimationMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;
  
  virtual SendAudioRequest SendAudio(
      const SendAudioMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;
  
  virtual SendChatActionRequest SendChatAction(
      const SendChatActionMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendContactRequest SendContact(
      const SendContactMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendDiceRequest SendDice(
      const SendDiceMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;
  
  virtual SendDocumentRequest SendDocument(
      const SendDocumentMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendLocationRequest SendLocation(
      const SendLocationMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendMessageRequest SendMessage(
      const SendMessageMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendPhotoRequest SendPhoto(
      const SendPhotoMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendPollRequest SendPoll(
      const SendPollMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;
  
  virtual SendVenueRequest SendVenue(
      const SendVenueMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendVideoNoteRequest SendVideoNote(
      const SendVideoNoteMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendVideoRequest SendVideo(
      const SendVideoMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;

  virtual SendVoiceRequest SendVoice(
      const SendVoiceMethod::Parameters& parameters,
      const RequestOptions& request_options) = 0;
};

}  // namespace telegram::bot

USERVER_NAMESPACE_END
