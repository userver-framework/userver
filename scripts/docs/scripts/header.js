const TRUNK_VERSION = "trunk/develop";
const NONE_VERSION = "none"

function loadVersions() {
    return import('/versions.js').then((imported_versions_module) => {
        versions = imported_versions_module.versions
        return versions
    }
    ).catch(() => {
        console.log("Versions loading failed")
    });
}


const addModal = () => {
    const modalHandler = () => {
        const modal = document.getElementById('main-nav')
        const closeBtn = document.getElementsByClassName('sm-dox')[0]
        if (window.innerWidth < 768) {
            closeBtn.appendChild(modal)
        } else {
            closeBtn.after(modal)
        }
    }

    window.addEventListener('resize', () => {
        modalHandler()
    })

    modalHandler()
}

const onBurger = () => {
    const burgerBtn = document.querySelector('.main-menu-btn');
    const modal = document.getElementById('navbar-main-menu');

    burgerBtn.addEventListener('click', () => {
        const isVisible = modal.style.display == 'flex';

        modal.style.display = isVisible ? null : 'flex';
    });
}

const create_nav_wrapper = () => {
    const searchBoxWrapper = document.getElementById('searchBoxPos2');
    const themeToggler = document.querySelector("doxygen-awesome-dark-mode-toggle");
    const searchBox = document.getElementById('MSearchBox');

    const mainNav = document.createElement('div');
    mainNav.id = 'main-navbar';

    const mainMenu = document.getElementById('main-menu');
    mainMenu.id = 'navbar-main-menu';

    mainNav.appendChild(mainMenu);

    mainMenu.after(searchBox);
    mainMenu.after(themeToggler);

    searchBoxWrapper.parentNode.removeChild(searchBoxWrapper);

    const oldWrapper = document.getElementById('main-nav');

    oldWrapper.before(mainNav);
}

const remove_legacy_searchbox = () => {
    const burgerBtn = document.querySelector('.main-menu-btn');
    const mainMenu = document.getElementById('navbar-main-menu');

    mainMenu.after(burgerBtn);

    const mobileSearchBox = document.getElementById('searchBoxPos1');

    mobileSearchBox.parentNode.removeChild(mobileSearchBox);
}

const old_docs_version = () => {
    const version = get_current_version()

    if (version != TRUNK_VERSION) {
        const brief = document.getElementById('projectbrief').getElementsByTagName('a')[0];
        brief.textContent += " " + version;
    }
    const base_page_url = "/"

    var warning = document.createElement("div");
    warning.style.width = '100%';
    warning.style.display = 'flex';
    warning.style.flexDirection = 'column';
    warning.innerHTML = `
      <a style="padding: 16px; margin-bottom: 20px; text-align: center; border: 1px solid var(--warning-color-dark); border-radius: var(--border-radius-large);" href="${base_page_url}">
        ⚠️ This is the documentation for an old userver version. Click here to switch to the latest version.
      </a>
    `;
    const titlearea = document.getElementById('titlearea');
    titlearea.parentNode.insertBefore(warning, titlearea);
}

const get_current_version = () => {
    const { pathname } = window.location;

    const urlSplitLimiter = 3;
    const versionTokenPosition = 2;

    if (pathname.startsWith("/docs")) {
        return pathname.split("/", urlSplitLimiter)[versionTokenPosition]
    }

    if (pathname.startsWith("/d4/de0/versions.html")) {
        return NONE_VERSION
    }

    return TRUNK_VERSION;
}

const get_latest_major_verision = (versions) => {
    latest_version = versions[versions.length - 1]
    latest_version_prefix = latest_version.substring(0, latest_version.indexOf("."))
    penultimate_element_position = versions.length - 2

    for (let i = penultimate_element_position; i >= 0; i--) {
        if (!versions[i].startsWith(latest_version_prefix)) {
            return versions[i + 1]
        }
    }
}

const get_latest_previous_major_version = (versions) => {
    latest_version = versions[versions.length - 1]
    latest_version_prefix = latest_version.substring(0, latest_version.indexOf("."))
    penultimate_element_position = versions.length - 2

    for (let i = penultimate_element_position; i >= 0; i--) {
        if (!versions[i].startsWith(latest_version_prefix)) {
            return versions[i]
        }
    }
}

const add_docs_versioning = () => {

    loadVersions().then((versions) => {

        const latest_version = versions[versions.length - 1]
        const current_version = get_current_version()
        let latest_major_version = get_latest_major_verision(versions)

        if (latest_version == latest_major_version) {
            latest_major_version = get_latest_previous_major_version(versions)
        }

        if (current_version != latest_version &&
            current_version != TRUNK_VERSION &&
            current_version != NONE_VERSION) {
            old_docs_version()
        }

        const footer = document.getElementById('nav-path').getElementsByTagName('ul')[0];
        const footer_prefix = `
        <li style="box-shadow: inset -1px 0 0 0 var(--separator-color); background-image: none; margin-right: 48px;">
        <span style="color: var(--toc-foreground);">Docs version:</span>
    `

        let footer_infix = ""

        if (current_version == TRUNK_VERSION) {
            footer_infix += `<span style="background-image: none; color: var(--toc-active-color); font-weight: bold;">${TRUNK_VERSION}</span>, `
        } else {
            footer_infix += `<a href="/index.html">${TRUNK_VERSION}</a>, `
        }

        if (current_version != latest_version) {
            footer_infix += `<a href="/docs/${latest_version}/index.html">${latest_version}</a>, `
        } else {
            footer_infix += `<span style="background-image: none; color: var(--toc-active-color); font-weight: bold;">${latest_version}</span>, `
        }

        if (current_version != latest_major_version) {
            footer_infix += `<a href="/docs/${latest_major_version}/index.html">${latest_major_version}</a>, `
        } else {
            footer_infix += `<span style="background-image: none; color: var(--toc-active-color); font-weight: bold;">${latest_major_version}</span>, `
        }

        footer_infix += `<a href="/d4/de0/versions.html">others</a>`

        footer.innerHTML = footer_prefix + footer_infix
            + footer.innerHTML;
    })

}

const init_header = () => {
    addModal();
    create_nav_wrapper();
    remove_legacy_searchbox();
    onBurger();
    add_docs_versioning();
}
