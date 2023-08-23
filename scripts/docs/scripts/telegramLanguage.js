function telegram_channel_language() {
  if (/^ru/.test(navigator.language)) {
    const channel = $("#telegram_channel");
    if (channel) {
      channel.attr("href", "https://t.me/userver_ru");
    }
  }
}

// We do not use $(function() { ... }); because
// * all the elements (except menu parts) are already parsed up above
// * skipping it speedups page load
// * with it Firefox does not scroll to a JS generated anchor links
telegram_channel_language();
