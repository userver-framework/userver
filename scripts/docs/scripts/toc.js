const LOWER_CASE_TRANSLITTERATION_MAPPING = {
  а: "a",
  б: "b",
  в: "v",
  г: "g",
  д: "d",
  е: "e",
  ё: "jo",
  ж: "zh",
  з: "z",
  и: "i",
  й: "jj",
  к: "k",
  л: "l",
  м: "m",
  н: "n",
  о: "o",
  п: "p",
  р: "r",
  с: "s",
  т: "t",
  у: "u",
  ф: "f",
  х: "x",
  ц: "c",
  ч: "ch",
  ш: "sh",
  щ: "shh",
  ъ: "'",
  ы: "y",
  ь: "'",
  э: "je",
  ю: "ju",
  я: "ja",
};
const DOXYGEN_DIAMOND_STRING = "◆\u00A0"; // ◆&nbsp;

function make_id(raw_id) {
  return raw_id
    .toLowerCase()
    .split("")
    .map(function (char) {
      return LOWER_CASE_TRANSLITTERATION_MAPPING[char] || char;
    })
    .join("")
    .replace(/\W/g, "");
}

function draw_toc() {
  const headers = document.querySelectorAll(
    ".contents h1, .contents h2, .contents h3, .contents h4, .contents h5, .contents h6"
  );

  if (!headers.length) return;

  let toc = '<div class="toc"><h3>Table of Contents</h3><ul>';

  for (const header of headers) {
    let text = header.textContent.trim();

    let level = header.nodeName.slice(1);

    if (text.startsWith(DOXYGEN_DIAMOND_STRING)) {
      level++;
      text = text.slice(DOXYGEN_DIAMOND_STRING.length);
    }

    let link =
      header.querySelector("a[id]")?.id ||
      header
        .querySelector("span.permalink > a")
        ?.getAttribute("href")
        ?.slice(1);

    if (!link) {
      const anchor = document.createElement("a");
      anchor.classList.add("anchor");
      anchor.id = make_id(header.textContent);
      header.appendChild(anchor);
      link = anchor.id;
    }

    toc += `<li class="level${level}"><a href="#${link}">${text}</a></li>`;
  }

  toc += "</ul></div>";

  $("<div>")
    .append([
      $(".header"),
      $("<div>")
        .addClass("contents")
        .append([toc, $(".contents").attr("class", "textblock")]),
    ])
    .insertAfter("#MSearchResultsWindow");
}

window.addEventListener("load", () => {
  if (document.getElementById("landing_logo_id") === null) {
    draw_toc();
  }
});
