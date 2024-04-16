function changeTelegramChannelLanguageForRussianSpeakingUser() {
  if (!isBrowserUseRussianLanguage()) {
    return;
  }

  getAllTelegramLinkElements().map(element => element.href = "https://t.me/userver_ru");
}

function getAllTelegramLinkElements() {
  return Array.from(document.querySelectorAll("a[href='https://t.me/userver_en']"));
}

function isBrowserUseRussianLanguage() {
  return /^ru/.test(navigator.language);
}