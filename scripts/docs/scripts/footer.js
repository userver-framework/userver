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

waitForElm('#MSearchField').then(() => {
    init_all_results_button();
    init_search_hotkey();
    init_search_observer();

    /* init_header() must run after initMenu() fills #main-menu; waiting for
     * dark-mode-toggle is too early because DarkModeToggle runs before initMenu
     * on DOMContentLoaded and would move an empty menu out of #main-nav. */
    waitForElm('#main-menu > li > a').then(() => {
        init_header();

        addLinks();
        changeTelegramChannelLanguageForRussianSpeakingUser();
    });
});

function hideEmptyPageNav() {
    const pageNav = document.getElementById('page-nav');
    if (!pageNav?.querySelector('ul.page-outline li')) {
        pageNav.style.display = 'none';
        const container = document.getElementById('container');
        if (container) {
            container.style.gridTemplateColumns = 'auto';
        }
    }
}

const isLanding = document.getElementById('landing_logo_id') !== null;
if (isLanding) {
    LandingFeedback.init();
} else {
    highlight_code();
    styleNavButtons();
    waitForElm('#page-nav-contents ul.page-outline').then(() => {
        hideEmptyPageNav();
        PageFeedback.init();
    });
}
