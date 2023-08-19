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

const init_header = () => {
    addModal();
    create_nav_wrapper();
    remove_legacy_searchbox();
    onBurger();
}
