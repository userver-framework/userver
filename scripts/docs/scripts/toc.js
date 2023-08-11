const LOWER_CASE_TRANSLITTERATION_MAPPING = {
    "а":"a", "б":"b", "в":"v", "г":"g", "д":"d", "е":"e", "ё":"jo", "ж":"zh",
    "з":"z", "и":"i", "й":"jj", "к":"k", "л":"l", "м":"m", "н":"n", "о":"o",
    "п":"p", "р":"r", "с":"s", "т":"t", "у":"u", "ф":"f", "х":"x", "ц":"c",
    "ч":"ch", "ш":"sh", "щ":"shh", "ъ":"\'", "ы":"y", "ь":"\'", "э":"je",
    "ю":"ju", "я":"ja"
};
const DOXYGEN_DIAMOND_STRING = '◆\u00A0' // ◆&nbsp;

function make_id(raw_id) {
    return raw_id.toLowerCase().split('').map(function (char) {
        return LOWER_CASE_TRANSLITTERATION_MAPPING[char] || char;
    }).join('').replace(/\W/g, '');
}

const html_escape = function () {
    const p = document.createElement('p');
    return function (text) {
        p.textContent = text;
        return p.innerHTML;
    };
}();

function draw_toc() {
    let headers = $(':header');
    if (headers.length === 0) {
        return;
    }

    let sidenav_content = '<div id="mySidenav" class="sidenav">';
    sidenav_content += '<h2>Table of contents</h2>';
    headers.each(function() {
        let index = parseInt(this.nodeName.substring(1)) - 1;
        const header = $(this);
        let id = header.attr('id');
        if (!id) {
          id = make_id(header.text())
          header.attr('id', id);
        }

        let header_text = header.text()
        if (header_text.startsWith(DOXYGEN_DIAMOND_STRING)) {
            ++index;
            header_text = header_text.substring(DOXYGEN_DIAMOND_STRING.length);
        }

        sidenav_content += ''
          + '<a href="#' + id + '" style="padding-left: ' + index * 15 + 'px">'
          + '•&nbsp;' + html_escape(header_text)
          + '</a>'
        ;
        header.append(' <a class="hoverlink" href="#' + id + '">🔗</a>')
    });
    sidenav_content += '</div>';

    $(sidenav_content).insertAfter('.header');
}

if (document.getElementById('landing_logo_id') === null) { // TODO: on page load
    draw_toc();
}