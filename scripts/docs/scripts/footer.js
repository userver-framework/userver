import { draw_toc } from "./toc.js";
import { changeTelegramChannelLanguageForRussianSpeakingUser } from "./telegramLanguage.js";
import { init_header } from "./header.js";
import { init_all_results_button, init_search_hotkey, init_search_observer } from "./search.js";
import { highlight_code } from "./codeHighlight.js";
import { styleNavButtons } from "./styledBtn.js";
import { LandingFeedback, PageFeedback } from "./feedback.js";

const addLinks = () =>  {
    const links = document.createElement('div')
    links.id = 'links';
    const logo_path = document.getElementById('projectlogo').getElementsByTagName('img')[0].src;
    const path = logo_path.substring(0, logo_path.lastIndexOf('/'));

    links.innerHTML = `
        <a href="https://github.com/userver-framework/" id='github_header' rel="noopener" target="_blank" class="titlelink">
            <img src="${path}/github_logo.svg" alt="Github" class="gh-logo"/>
        </a>
        <a href="https://t.me/userver_en" rel="noopener" id='telegram_channel' target="_blank" class="titlelink generic_tg_link">
            <img src="${path}/telegram_logo.svg" alt="Telegram"/>
        </a>
    `
    document.getElementById('main-navbar').appendChild(links);
}

function waitForElm(selector) {
    return new Promise(resolve => {
        if (document.querySelector(selector)) {
            return resolve(document.querySelector(selector));
        }

        const observer = new MutationObserver(mutations => {
            if (document.querySelector(selector)) {
                observer.disconnect();
                resolve(document.querySelector(selector));
            }
        });

        /* If you get "parameter 1 is not of type 'Node'" error, see https://stackoverflow.com/a/77855838/492336 */
        observer.observe(document.body, {
            childList: true,
            subtree: true
        });
    });
}

waitForElm('#MSearchField').then((elm) => {
    init_all_results_button();
    init_search_hotkey();
    init_search_observer();

    /* Following actions require '#MSearchField' AND 'doxygen-awesome-dark-mode-toggle' */
    waitForElm('doxygen-awesome-dark-mode-toggle').then((elm) => {
        init_header();

        addLinks();
        changeTelegramChannelLanguageForRussianSpeakingUser();
    });
});

const isLanding = document.getElementById('landing_logo_id') !== null;
if (isLanding) {
    LandingFeedback.init();
} else {
    draw_toc();
    highlight_code();
    styleNavButtons();
    DoxygenAwesomeInteractiveToc.init();
    PageFeedback.init();
}
