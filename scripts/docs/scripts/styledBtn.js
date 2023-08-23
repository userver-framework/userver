function styleNavButtons() {
    const navButtonsContainer = document.querySelector('.bottom-nav')
    const navButtons = document.querySelectorAll('.bottom-nav .el');
    
    if (!(navButtonsContainer && navButtons.length)) return;
    
    for (const button of navButtons) {
        const isBackButton = button.previousSibling.textContent.includes('⇦');
        button.className = `button ${isBackButton ? 'prev' : 'next'}`;
        button.innerHTML = `<span class="btn-sub">${isBackButton ? 'Go back' : 'Up next'}</span><span class="btn-title">${button.innerHTML}</span>`;
    }
    
    navButtonsContainer.innerHTML = '';
    navButtons.forEach(button => navButtonsContainer.appendChild(button));
}
