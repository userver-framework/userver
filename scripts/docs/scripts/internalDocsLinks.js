const WIKI_URL = "https://nda.ya.ru/t/IomTKy5A7s3BdU";
const INTERNAL_CHAT_URL = "https://nda.ya.ru/t/a9hkC3Sp7rtSbi";
const YC_LINK_URL = "https://nda.ya.ru/t/jiHqZjLt7rtT8H";
const HEADER_SOCIAL_ICON_PX = 32;

const WIKI_ICON_PATH =
  "M 25 4.5 C 25.5523 4.5 26 4.94772 26 5.5 V 26.5 C 26 27.0523 25.5523 27.5 25 27.5 H 10 C 7.79086 27.5 6 25.7091 6 23.5 V 8.5 C 6 6.29086 7.79086 4.5 10 4.5 H 25 Z M 10.5 21.5 C 9.39543 21.5 8.5 22.3954 8.5 23.5 C 8.5 24.6046 9.39543 25.5 10.5 25.5 H 23.5 C 23.7761 25.5 24 25.2761 24 25 V 21.5 H 10.5 Z M 11.5 12 C 11.2239 12 11 12.2239 11 12.5 V 13.5 C 11 13.7761 11.2239 14 11.5 14 H 21.5 C 21.7761 14 22 13.7761 22 13.5 V 12.5 C 22 12.2239 21.7761 12 21.5 12 H 11.5 Z M 11.5 8 C 11.2239 8 11 8.22386 11 8.5 V 9.5 C 11 9.77614 11.2239 10 11.5 10 H 21.5 C 21.7761 10 22 9.77614 22 9.5 V 8.5 C 22 8.22386 21.7761 8 21.5 8 H 11.5 Z";

const YC_ICON_BACKGROUND_PATH =
  "m27.08 22.109.002.006c.336 1.444.243 2.47-.203 3.134-.44.654-1.27 1.019-2.57 1.032-1.029 0-1.825-.478-2.439-1.072-.555-.537-.947-1.157-1.216-1.583l-.084-.133c-.201-.323-.593-.761-1.112-1.175a6.14 6.14 0 0 0-1.926-1.054c-.747-.238-1.58-.313-2.438-.069-.595.17-1.193.49-1.775 1.003.492-.965 1.29-1.796 2.27-2.458 1.3-.879 2.912-1.45 4.52-1.636 1.608-.187 3.195.015 4.455.661 1.254.644 2.192 1.73 2.517 3.344ZM14.586 9.318c.733-1.525 2.1-2.301 3.526-2.45 1.63-.172 3.316.475 4.218 1.742.641 1.01.967 1.888 1.006 2.64a2.43 2.43 0 0 1-.72 1.898c-.922.611-1.75.525-2.409.192a3.395 3.395 0 0 1-1.384-1.345c-.659-1.446-1.388-2.406-2.21-2.803-.66-.318-1.341-.26-2.027.126Z";

const YC_ICON_FOREGROUND_PATH =
  "M15.68 3.718c2.163 0 4.2 1.322 6.104 4.066-.896-.473-1.816-.703-2.757-.682-1.239.027-2.492.49-3.758 1.365l-.01.007c-1.56 1.224-2.068 2.416-2.483 3.582-.206.576-.629 1.89-.55 3.116.04.616.21 1.23.616 1.717.411.492 1.043.826 1.957.92h.01c1.255.063 2.203-.432 3.135-.963l.2-.114c.87-.499 1.75-1.003 2.92-1.18 1.867-.215 2.901.72 3.147 1.06l.009.01c1.09 1.32 1.908 2.723 2.457 4.211a6.184 6.184 0 0 0-1.4-1.132c-1.44-.861-2.592-.942-3.956-.942-1.712 0-4.083.524-6.043 1.735-.965.597-1.506 1.005-1.93 1.456-.352.376-.62.779-.983 1.326l-.22.33c-.432.645-1.114 1.463-1.94 2.036-.415.249-.675.362-.989.42-.328.06-.724.06-1.427.06-.814 0-1.697-.396-2.308-1.067-.605-.664-.936-1.589-.67-2.657C7.445 13.201 9.603 8.525 11.386 6.15c.889-1.183 1.677-1.785 2.374-2.094.696-.31 1.323-.337 1.918-.337Z";

const SVG_NAMESPACE = "http://www.w3.org/2000/svg";

const createSvgIconLink = (
  href,
  {
    id,
    className,
    pathD,
    viewBox,
    title,
    iconFill = "currentColor",
    iconSizePx = HEADER_SOCIAL_ICON_PX,
  }
) => {
  const link = document.createElement("a");
  link.href = href;
  link.rel = "noopener";
  link.target = "_blank";
  if (id) {
    link.id = id;
  }
  link.className = className;
  if (title) {
    link.title = title;
  }

  const svg = document.createElementNS(SVG_NAMESPACE, "svg");
  svg.setAttribute("xmlns", SVG_NAMESPACE);
  svg.setAttribute("width", String(iconSizePx));
  svg.setAttribute("height", String(iconSizePx));
  svg.setAttribute("viewBox", viewBox);
  svg.setAttribute("fill", "none");
  svg.setAttribute("aria-hidden", "true");

  const path = document.createElementNS(SVG_NAMESPACE, "path");
  path.setAttribute("d", pathD);
  path.setAttribute("fill", iconFill);
  svg.appendChild(path);
  link.appendChild(svg);

  return link;
};

const createYcIconPath = ({ fill, stroke, pathD }) => {
  const path = document.createElementNS(SVG_NAMESPACE, "path");
  path.setAttribute("fill", fill);
  path.setAttribute("stroke", stroke);
  path.setAttribute("stroke-width", "0.436");
  path.setAttribute("d", pathD);
  return path;
};

const createYcIconLink = (href, { id, className, title }) => {
  const link = document.createElement("a");
  link.href = href;
  link.rel = "noopener";
  link.target = "_blank";
  if (id) {
    link.id = id;
  }
  link.className = className;
  if (title) {
    link.title = title;
  }

  const svg = document.createElementNS(SVG_NAMESPACE, "svg");
  svg.setAttribute("xmlns", SVG_NAMESPACE);
  svg.setAttribute("width", "32");
  svg.setAttribute("height", "32");
  svg.setAttribute("fill", "none");
  svg.setAttribute("aria-hidden", "true");

  const background = document.createElementNS(SVG_NAMESPACE, "rect");
  background.setAttribute("width", "32");
  background.setAttribute("height", "32");
  background.setAttribute("fill", "#B6B8EE");
  background.setAttribute("rx", "16");

  svg.appendChild(background);
  svg.appendChild(
    createYcIconPath({
      fill: "#4E509E",
      stroke: "#4E509E",
      pathD: YC_ICON_BACKGROUND_PATH,
    })
  );
  svg.appendChild(
    createYcIconPath({
      fill: "#E0E1FF",
      stroke: "#E0E1FF",
      pathD: YC_ICON_FOREGROUND_PATH,
    })
  );
  link.appendChild(svg);

  return link;
};

const addWikiHeaderLink = () => {
  const links = document.getElementById("links");
  if (!links || links.querySelector("#wiki_menu_link")) {
    return;
  }

  const wikiLink = createSvgIconLink(WIKI_URL, {
    id: "wiki_menu_link",
    className: "titlelink wiki-menu-link",
    pathD: WIKI_ICON_PATH,
    viewBox: "0 0 32 32",
    title: "Internal wiki",
    iconFill: "#FFFFFF",
  });

  const telegramLink = document.getElementById("telegram_channel");
  if (telegramLink) {
    links.insertBefore(wikiLink, telegramLink);
  } else {
    links.appendChild(wikiLink);
  }
};

const removeGithubHeaderLink = () => {
  document.getElementById("github_header")?.remove();
};

const addYcHeaderLink = () => {
  const links = document.getElementById("links");
  if (!links || links.querySelector("#yc_header")) {
    return;
  }

  const ycLink = createYcIconLink(YC_LINK_URL, {
    id: "yc_header",
    className: "titlelink",
    title: "Yandex Cloud",
  });

  const telegramLink = document.getElementById("telegram_channel");
  if (telegramLink) {
    links.insertBefore(ycLink, telegramLink);
  } else {
    links.appendChild(ycLink);
  }
};

const updateChatLink = () => {
  const chatLink = document.getElementById("telegram_channel");
  if (!chatLink) {
    return;
  }

  chatLink.href = INTERNAL_CHAT_URL;
};

export function configureInternalDocsLinks() {
  removeGithubHeaderLink();
  addYcHeaderLink();
  addWikiHeaderLink();
  updateChatLink();
}
