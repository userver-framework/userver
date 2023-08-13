const addLinks = () =>  {
    const links = document.getElementById('links')
    document.getElementById('main-nav').after(links)
    const toggler = document.querySelector('[title="Toggle Light/Dark Mode"]')
    document.getElementById('MSearchBox').before(toggler)

}
