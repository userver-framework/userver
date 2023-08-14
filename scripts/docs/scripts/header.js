const addLinks = () =>  {
    const links = document.createElement('div')
    links.setAttribute('style', 'padding-right: 20px;')
    links.setAttribute('id', 'links')
    links.innerHTML = `
        <a href="https://github.com/userver-framework/" rel="noopener" target="_blank" class="titlelink">
            <img src="./github_logo.svg" alt="Github" class="gh-logo"/> 
        </a>
        <a href="https://t.me/userver_en" rel="noopener" id='telegram_channel' target="_blank" class="titlelink">
            <img src="./telegram_logo.svg" alt="Telegram"/>
        </a>
    `
    document.getElementById('main-nav').after(links)
    const toggler = document.querySelector('[title="Toggle Light/Dark Mode"]')
    document.getElementById('MSearchBox').before(toggler)
  

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