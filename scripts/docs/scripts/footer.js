import { changeTelegramChannelLanguageForRussianSpeakingUser } from "./telegramLanguage.js";
import { init_header } from "./header.js";
import { init_all_results_button, init_search_hotkey, init_search_observer, init_search_results_anchor } from "./search.js";
import { highlight_code } from "./codeHighlight.js";
import { styleNavButtons } from "./styledBtn.js";
import { LandingFeedback, PageFeedback } from "./feedback.js";

const addLink = (container, href, { id, className, imgSrc, imgAlt, imgClass } = {}) => {
    const link = document.createElement('a');
    link.href = href;
    link.rel = 'noopener';
    link.target = '_blank';
    link.id = id;
    link.className = className;
    
    const img = document.createElement('img');
    img.src = imgSrc;
    img.alt = imgAlt;
    if (imgClass) {
        img.className = imgClass;
    }
    link.appendChild(img);
    container.appendChild(link);
};

const addLinks = () => {
    const links = document.createElement('div');
    links.id = 'links';
    const logo_path = document.getElementById('projectlogo').getElementsByTagName('img')[0].src;
    const path = logo_path.substring(0, logo_path.lastIndexOf('/'));

    addLink(links, 'https://github.com/userver-framework/', {
        id: 'github_header',
        className: 'titlelink',
        imgSrc: `${path}/github_logo.svg`,
        imgAlt: 'Github',
        imgClass: 'gh-logo',
    });
    addLink(links, 'https://t.me/userver_en', {
        id: 'telegram_channel',
        className: 'titlelink generic_tg_link',
        imgSrc: `${path}/telegram_logo.svg`,
        imgAlt: 'Telegram',
    });

    document.getElementById('main-navbar').appendChild(links);
};

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
    init_search_results_anchor();
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
        container?.classList.add('page-nav-hidden');
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
