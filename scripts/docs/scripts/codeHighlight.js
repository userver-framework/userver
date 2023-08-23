function highlight_code() {
    if (window.location.href.indexOf("pp_source") !== -1
        || window.location.href.indexOf("cpp-example") !== -1) {
        // Header listing is already nicely highlighted by Doxygen
        return;
    }

    hljs.configure({
        tabReplace: '    ' // 4 spaces
    });
    hljs.registerAliases('sh', {languageName: 'shell'});
    hljs.registerAliases('bash', {languageName: 'shell'});
    hljs.registerAliases('yml', {languageName: 'yaml'});
    $(".fragment").each(function() {
        const node = $(this);
        let data = '';
        let language = '';
        let requires_highlighting = true;

        node.children('div.line').each(function(i) {
            line = $(this).text();

            // Doxygen 1.8.11 workaround to remove line numbers.
            // No line numbers added in Doxygen 1.8.13
            line = line.replace(/^ *\d+\xA0/, '');

            if (i === 0) {
                line = line.trimLeft().replace(/^# /, '');
                if (line === 'autodetect') {
                    return true;
                } else if (hljs.getLanguage(line)) {
                    language = line;
                    return true;
                } else {
                    requires_highlighting = false;
                    return false;
                }
            }

            data += line + '\n';
        });

        if (requires_highlighting === false) {
            return;
        }

        if (language !== '') {
            data = hljs.highlight(data, { language }).value;
        } else {
            data = hljs.highlightAuto(data).value;
        }
        node.replaceWith('<div class="fragment"><pre>' + data + '</pre></div>');
    });
}
