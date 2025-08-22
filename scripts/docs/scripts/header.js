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

    var warning = document.createElement("div");
    warning.style.width = '100%';
    warning.style.display = 'flex';
    warning.style.flexDirection = 'column';
    warning.innerHTML = `
      <a style="padding: 16px; margin-bottom: 20px; text-align: center; border: 1px solid var(--warning-color-dark); border-radius: var(--border-radius-large);" href="/">
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

    if (pathname.startsWith("/docs/")) {
        return pathname.split("/", urlSplitLimiter)[versionTokenPosition]
    }

    if (pathname.startsWith("/d4/de0/versions.html")) {
        return NONE_VERSION
    }

    return TRUNK_VERSION;
}

const getPreviousVersion = (versions, current_version) => {
    let index = -1;

    if (typeof current_version.index === "number") {
        index = current_version.index;
    } else if ("value" in current_version) {
        index = versions.indexOf(current_version.value);
    }

    if (index <= 0 || index >= versions.length) {
        return undefined;
    }

    return {
        index: index - 1,
        value: versions[index - 1]
    };
}

const getVersionMajorPart = (version) => {
    const major = version.split(".")[0].replace("v", "");

    const major_as_number = parseInt(major, 10);

    return major_as_number;
};


const floor_to_major_version = (versions, target_version) => {
    let target_major_version = getVersionMajorPart(target_version)
    let version_iterator = { value: target_version, index: versions.length - 1 }

    while (getVersionMajorPart(getPreviousVersion(versions, version_iterator).value) == target_major_version) {
        version_iterator = getPreviousVersion(versions, version_iterator)
    }

    return version_iterator.value
}

const add_docs_versioning = () => {

    loadVersions().then((versions) => {
        const latest_version = versions[versions.length - 1]
        let second_likely_popular_version = floor_to_major_version(versions, latest_version)

        if (second_likely_popular_version == latest_version) {
            second_likely_popular_version = getPreviousVersion(versions, { value: latest_version }).value
        }

        const current_version = get_current_version()

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

        not_trunk_versions = [latest_version, second_likely_popular_version]

        for (let version of not_trunk_versions) {
            if (current_version != version) {
                footer_infix += `<a href="/docs/${version}/index.html">${version}</a>, `
            } else {
                footer_infix += `<span style="background-image: none; color: var(--toc-active-color); font-weight: bold;">${version}</span>, `
            }
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
